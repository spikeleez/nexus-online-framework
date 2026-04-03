// Copyright Spike Plugins 2026. All Rights Reserved.

#include "Beacons/NexusPartyBeaconHost.h"
#include "Beacons/NexusPartyBeaconClient.h"

ANexusPartyBeaconHost::ANexusPartyBeaconHost(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// ClientBeaconActorClass tells AOnlineBeaconHost what actor to spawn when a
	// matching client connects. BeaconTypeName is the routing key — it must exactly
	// match the BeaconType string set in ANexusPartyBeaconClient's constructor.
	ClientBeaconActorClass = ANexusPartyBeaconClient::StaticClass();
	BeaconTypeName = ANexusPartyBeaconClient::StaticClass()->GetName();
}

void ANexusPartyBeaconHost::OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	// Called when a new beacon client actor is spawned for a connecting client.
	// Identity is unknown at this point — we wait for ServerRequestJoin before
	// adding the client to ConnectedClients or the party state.
	if (!Cast<ANexusPartyBeaconClient>(NewClientActor))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyHost] ClientConnected: actor is not ANexusPartyBeaconClient. Ignoring."));
		return;
	}
	
	NEXUS_LOG(LogNexus, Verbose, TEXT("[PartyHost] New client connected; awaiting ServerRequestJoin."));
}

void ANexusPartyBeaconHost::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	ANexusPartyBeaconClient* PartyClient = Cast<ANexusPartyBeaconClient>(LeavingClientActor);
	if (!IsValid(PartyClient))
	{
		Super::NotifyClientDisconnected(LeavingClientActor);
		return;
	}
	
	const FUniqueNetIdRepl& LeavingId = PartyClient->GetLocalPlayerId();
	
	// Only process if this client had already joined (has a valid ID and is in the party state).
	if (bPartyActive && LeavingId.IsValid())
	{
		FNexusPartySlot* Slot = PartyState.Members.FindByPredicate([&LeavingId](const FNexusPartySlot& S)
		{
			return S.IsActive() && S.MemberId == LeavingId;
		});
		
		if (Slot)
		{
			// Treat an unexpected disconnect the same as a voluntary leave.
			Slot->Status = ENexusPartyMemberStatus::Left;
			
			NEXUS_LOG(LogNexus, Log, TEXT("[PartyHost] Member %s disconnected unexpectedly (Left)."), *LeavingId.ToString());
			
			RemoveConnectedClient(PartyClient);
			NativeOnMemberLeftEvent.Broadcast(LeavingId, ENexusPartyMemberStatus::Left);
			BroadcastPartyState();
			NativeOnStateChangedEvent.Broadcast(PartyState);
		}
		else
		{
			// Client connected but never completed ServerRequestJoin — just remove silently
			RemoveConnectedClient(PartyClient);
		}
	}
	
	Super::NotifyClientDisconnected(LeavingClientActor);
}

void ANexusPartyBeaconHost::InitializeParty(const FUniqueNetIdRepl& LeaderId, const FString& LeaderName, int32 InMaxSize)
{
	if (!LeaderId.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyHost] InitializeParty: invalid LeaderId."));
		return;
	}
	
	// Reset to a known-clean state.
	PartyState = FNexusPartyState();
	PartyState.LeaderId = LeaderId;
	PartyState.LeaderDisplayName = LeaderName;
	PartyState.MaxSize = FMath::Clamp(InMaxSize, 2, 8);
	bPartyActive = true;
	
	NEXUS_LOG(LogNexus, Log, TEXT("[PartyHost] Party initialized. Leader=%s, MaxSize=%d"), *LeaderId.ToString(), PartyState.MaxSize);
	NativeOnStateChangedEvent.Broadcast(PartyState);
}

void ANexusPartyBeaconHost::DisbandParty()
{
	if (!bPartyActive)
	{
		return;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyHost] Disbanding party. Notifying %d connected client(s)."), ConnectedClients.Num());

	// Notify all connected members so they can clean up their local state
	for (ANexusPartyBeaconClient* Client : ConnectedClients)
	{
		if (IsValid(Client))
		{
			Client->ClientReceiveKick();
		}
	}

	ConnectedClients.Empty();
	PartyState = FNexusPartyState();
	bPartyActive = false;
}

bool ANexusPartyBeaconHost::KickMember(const FUniqueNetIdRepl& MemberId)
{
	if (!bPartyActive)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("[PartyHost] KickMember: party not active."));
		return false;
	}

	if (!MemberId.IsValid())
	{
		return false;
	}

	FNexusPartySlot* Slot = PartyState.Members.FindByPredicate([&MemberId](const FNexusPartySlot& S)
	{
		return S.IsActive() && S.MemberId == MemberId;
	});

	if (!Slot)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("[PartyHost] KickMember: member %s not found."), *MemberId.ToString());
		return false;
	}

	Slot->Status = ENexusPartyMemberStatus::Kicked;

	// Notify and disconnect the kicked client
	ANexusPartyBeaconClient* Client = FindConnectedClient(MemberId);
	if (IsValid(Client))
	{
		Client->ClientReceiveKick();
		RemoveConnectedClient(Client);
	}

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyHost] Kicked member %s."), *MemberId.ToString());

	NativeOnMemberLeftEvent.Broadcast(MemberId, ENexusPartyMemberStatus::Kicked);
	BroadcastPartyState();
	NativeOnStateChangedEvent.Broadcast(PartyState);

	return true;
}

void ANexusPartyBeaconHost::ProcessJoinRequest(ANexusPartyBeaconClient* Client, const FUniqueNetIdRepl& PlayerId, const FString& DisplayName)
{
	if (!IsValid(Client) || !PlayerId.IsValid())
    {
        NEXUS_LOG(LogNexus, Warning, TEXT("[PartyHost] ProcessJoinRequest: invalid client or PlayerId."));
        return;
    }

    if (!bPartyActive)
    {
        NEXUS_LOG(LogNexus, Log, TEXT("[PartyHost] ProcessJoinRequest: party not active. Rejecting %s."), *PlayerId.ToString());
        SendJoinResult(Client, ENexusPartyResult::InvalidState);
        return;
    }

    if (PartyState.IsLeader(PlayerId))
    {
        // The leader should never connect as a client to their own host
        NEXUS_LOG(LogNexus, Warning, TEXT("[PartyHost] ProcessJoinRequest: leader tried to join as member. Rejected."));
        SendJoinResult(Client, ENexusPartyResult::AlreadyInParty);
        return;
    }

    if (PartyState.IsActiveMember(PlayerId))
    {
        NEXUS_LOG(LogNexus, Warning, TEXT("[PartyHost] ProcessJoinRequest: %s already in party."), *PlayerId.ToString());
        SendJoinResult(Client, ENexusPartyResult::AlreadyInParty);
        return;
    }

    if (PartyState.IsFull())
    {
        NEXUS_LOG(LogNexus, Log, TEXT("[PartyHost] ProcessJoinRequest: party full. Rejecting %s."), *PlayerId.ToString());
        SendJoinResult(Client, ENexusPartyResult::PartyFull);
        return;
    }

    // Accept: stamp identity on the server-side client proxy and update state
    Client->SetLocalPlayerId(PlayerId);
    Client->SetLocalDisplayName(DisplayName);

    PartyState.Members.Emplace(PlayerId, DisplayName);
    ConnectedClients.AddUnique(Client);

    NEXUS_LOG(LogNexus, Log, TEXT("[PartyHost] Member %s ('%s') accepted. Party: %d/%d."), *PlayerId.ToString(), *DisplayName, PartyState.GetActiveCount(), PartyState.MaxSize);

    // Send accept + current full state to the joining client
    SendJoinResult(Client, ENexusPartyResult::Success);

    // Broadcast updated state to ALL connected members (including the new one)
    BroadcastPartyState();

    NativeOnMemberJoinedEvent.Broadcast(PlayerId, PartyState);
    NativeOnStateChangedEvent.Broadcast(PartyState);
}

void ANexusPartyBeaconHost::ProcessLeaveRequest(ANexusPartyBeaconClient* Client)
{
	if (!IsValid(Client) || !bPartyActive)
	{
		return;
	}

	const FUniqueNetIdRepl LeavingId = Client->GetLocalPlayerId();
	if (!LeavingId.IsValid())
	{
		return;
	}

	FNexusPartySlot* Slot = PartyState.Members.FindByPredicate([&LeavingId](const FNexusPartySlot& S)
	{
		return S.IsActive() && S.MemberId == LeavingId;
	});

	if (Slot)
	{
		Slot->Status = ENexusPartyMemberStatus::Left;
	}

	RemoveConnectedClient(Client);

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyHost] Member %s voluntarily left."), *LeavingId.ToString());

	NativeOnMemberLeftEvent.Broadcast(LeavingId, ENexusPartyMemberStatus::Left);
	BroadcastPartyState();
	NativeOnStateChangedEvent.Broadcast(PartyState);
}

void ANexusPartyBeaconHost::BroadcastPartyState()
{
	for (ANexusPartyBeaconClient* Client : ConnectedClients)
	{
		if (IsValid(Client))
		{
			Client->ClientReceivePartyState(PartyState);
		}
	}
}

void ANexusPartyBeaconHost::SendJoinResult(ANexusPartyBeaconClient* Client, ENexusPartyResult PartyResult)
{
	if (IsValid(Client))
	{
		// Pass current state on success; empty state on failure (no information leak)
		Client->ClientReceiveJoinResult(PartyResult, PartyResult == ENexusPartyResult::Success ? PartyState : FNexusPartyState());
	}
}

ANexusPartyBeaconClient* ANexusPartyBeaconHost::FindConnectedClient(const FUniqueNetIdRepl& PlayerId) const
{
	for (ANexusPartyBeaconClient* Client : ConnectedClients)
	{
		if (IsValid(Client) && Client->GetLocalPlayerId() == PlayerId)
		{
			return Client;
		}
	}
	return nullptr;
}

bool ANexusPartyBeaconHost::RemoveConnectedClient(const ANexusPartyBeaconClient* Client)
{
	const int32 Removed = ConnectedClients.RemoveAll(
	[Client](const TObjectPtr<ANexusPartyBeaconClient>& Ptr)
	{
		return Ptr.Get() == Client;
	});
	return Removed > 0;
}
