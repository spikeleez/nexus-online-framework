// Copyright Spike Plugins 2026. All Rights Reserved.

#include "Beacons/NexusPartyBeaconClient.h"
#include "Beacons/NexusPartyBeaconHost.h"
#include "NexusOnlineSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusPartyBeaconClient)

ANexusPartyBeaconClient::ANexusPartyBeaconClient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void ANexusPartyBeaconClient::OnConnected()
{
	NEXUS_LOG(LogNexus, Log, TEXT("[PartyClient] Connected. Requesting to join as '%s'."), *LocalDisplayName);

	// Identity is already validated in ConnectToHost, so send immediately.
	ServerRequestJoin(LocalPlayerId, LocalDisplayName);
}

void ANexusPartyBeaconClient::OnFailure()
{
	NEXUS_LOG(LogNexus, Warning, TEXT("[PartyClient] Connection failed or timed out."));
	NativeOnJoinResultEvent.Broadcast(ENexusPartyResult::ConnectionFailed, FNexusPartyState());
}

bool ANexusPartyBeaconClient::ConnectToHost(const FString& HostAddress, int32 Port)
{
	if (HostAddress.IsEmpty())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyClient] ConnectToHost: empty HostAddress."));
		return false;
	}

	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyClient] ConnectToHost: LocalPlayerId not set. Call SetLocalPlayerId first."));
		return false;
	}

	const int32 EffectivePort = (Port > 0) ? Port : UNexusOnlineSettings::Get()->BeaconListenPort;
	const FString FullAddress = FString::Printf(TEXT("%s:%d"), *HostAddress, EffectivePort);

	FURL URL(nullptr, *FullAddress, TRAVEL_Absolute);
	const bool bInitiated = InitClient(URL);

	if (bInitiated)
	{
		NEXUS_LOG(LogNexus, Log, TEXT("[PartyClient] Connecting to party at '%s'."), *FullAddress);
	}
	else
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyClient] InitClient failed for '%s'."), *FullAddress);
	}

	return bInitiated;
}

bool ANexusPartyBeaconClient::ServerRequestJoin_Validate(const FUniqueNetIdRepl& PlayerId, const FString& DisplayName)
{
	// Reject obviously invalid requests early, before ProcessJoinRequest.
	return PlayerId.IsValid() && !DisplayName.IsEmpty() && DisplayName.Len() <= 64;
}

void ANexusPartyBeaconClient::ServerRequestJoin_Implementation(const FUniqueNetIdRepl& PlayerId, const FString& DisplayName)
{
	// Executed on the SERVER (server-side proxy of this actor).
	// Forward to the authoritative host object.
	ANexusPartyBeaconHost* PartyBeaconHost = Cast<ANexusPartyBeaconHost>(GetBeaconOwner());
	if (!IsValid(PartyBeaconHost))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyClient] [Server] ServerRequestJoin: could not get ANexusPartyBeaconHostObject."));
		ClientReceiveJoinResult(ENexusPartyResult::InvalidState, FNexusPartyState());
		return;
	}

	PartyBeaconHost->ProcessJoinRequest(this, PlayerId, DisplayName);
}

void ANexusPartyBeaconClient::ServerLeaveParty_Implementation()
{
	// Executed on the SERVER.
	ANexusPartyBeaconHost* PartyBeaconHost = Cast<ANexusPartyBeaconHost>(GetBeaconOwner());
	if (IsValid(PartyBeaconHost))
	{
		PartyBeaconHost->ProcessLeaveRequest(this);
	}
}

void ANexusPartyBeaconClient::ClientReceiveJoinResult_Implementation(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState)
{
	NEXUS_LOG(LogNexus, Log, TEXT("[PartyClient] Join result: %s."), *FString(LexToString(PartyResult)));

	if (PartyResult == ENexusPartyResult::Success)
	{
		CachedPartyState = PartyState;
	}

	NativeOnJoinResultEvent.Broadcast(PartyResult, PartyState);
}

void ANexusPartyBeaconClient::ClientReceivePartyState_Implementation(const FNexusPartyState& PartyState)
{
	NEXUS_LOG(LogNexus, Verbose, TEXT("[PartyClient] Party state updated: %d/%d active."), PartyState.GetActiveCount(), PartyState.MaxSize);

	CachedPartyState = PartyState;
	NativeOnStateUpdatedEvent.Broadcast(PartyState);
}

void ANexusPartyBeaconClient::ClientReceiveKick_Implementation()
{
	NEXUS_LOG(LogNexus, Log, TEXT("[PartyClient] Kicked from the party."));
	CachedPartyState = FNexusPartyState();
	NativeOnKickedEvent.Broadcast();
}
