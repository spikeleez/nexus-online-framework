// Copyright Spike Plugins 2026. All Rights Reserved.

#include "Managers/NexusPartyManager.h"
#include "Managers/NexusBeaconManager.h"
#include "Beacons/NexusPartyBeaconClient.h"
#include "Beacons/NexusPartyBeaconHost.h"
#include "OnlineSubsystemUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusPartyManager)

UNexusPartyManager::UNexusPartyManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNexusPartyManager::Initialize(UGameInstance* InGameInstance, UNexusBeaconManager* InBeaconManager)
{
	check(InGameInstance);
	check(InBeaconManager);

	GameInstance = InGameInstance;
	BeaconManager = InBeaconManager;

	// Cache the party host object if the beacon host is already running.
	// If not, it will be resolved on the first CreateParty() call.
	PartyHost = InBeaconManager->GetPartyHost();

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Initialized."));
}

void UNexusPartyManager::Deinitialize()
{
	// Clean shutdown: disband or leave before releasing resources
	if (IsPartyLeader())
	{
		DisbandParty();
	}
	else if (IsInParty())
	{
		LeaveParty();
	}

	UnbindPartyHostDelegates();
	CleanupPartyClient();

	PartyHost = nullptr;
	GameInstance.Reset();
	BeaconManager.Reset();

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Deinitialized."));
}

bool UNexusPartyManager::CreateParty(int32 MaxSize)
{
	 if (IsInParty())
    {
        NEXUS_LOG(LogNexus, Warning, TEXT("[PartyManager] CreateParty: already in a party. Disband or leave first."));
        OnPartyCreatedEvent.Broadcast(ENexusPartyResult::AlreadyInParty, FNexusPartyState());
        return false;
    }

    if (!BeaconManager.IsValid())
    {
        NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] CreateParty: BeaconManager is invalid."));
        OnPartyCreatedEvent.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
        return false;
    }

    // Ensure the shared beacon host is running
    if (!BeaconManager->IsBeaconHostActive())
    {
        if (!BeaconManager->StartBeaconHost())
        {
            NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] CreateParty: failed to start beacon host."));
            OnPartyCreatedEvent.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
            return false;
        }
    }

    // Refresh reference — host object is spawned when the beacon host starts
    PartyHost = BeaconManager->GetPartyHost();
    if (!IsValid(PartyHost))
    {
        NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] CreateParty: ANexusPartyBeaconHostObject not found on BeaconManager."));
        OnPartyCreatedEvent.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
        return false;
    }

    FUniqueNetIdRepl LocalId;
    FString LocalName;
    if (!GetLocalPlayerInfo(LocalId, LocalName))
    {
        NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] CreateParty: could not retrieve local player identity."));
        OnPartyCreatedEvent.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
        return false;
    }

    BindPartyHostDelegates();
    PartyHost->InitializeParty(LocalId, LocalName, MaxSize);

    // InitializeParty fires NativeOnStateChanged synchronously, which updates CachedPartyState
    // via OnHostStateChanged. Read it back for the broadcast below.
    CachedPartyState = PartyHost->GetPartyState();

    NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Party created. Leader=%s, MaxSize=%d."), *LocalId.ToString(), CachedPartyState.MaxSize);

    OnPartyCreatedEvent.Broadcast(ENexusPartyResult::Success, CachedPartyState);
    return true;
}

void UNexusPartyManager::DisbandParty()
{
	if (!IsPartyLeader())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("[PartyManager] DisbandParty: not the party leader."));
		return;
	}

	if (IsValid(PartyHost))
	{
		PartyHost->DisbandParty();
	}

	UnbindPartyHostDelegates();
	CachedPartyState = FNexusPartyState();

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Party disbanded."));
	OnPartyDisbandedEvent.Broadcast(ENexusPartyResult::Success);
}

bool UNexusPartyManager::KickMember(const FUniqueNetIdRepl& MemberId)
{
	if (!IsPartyLeader())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("[PartyManager] KickMember: not the party leader."));
		return false;
	}

	if (!IsValid(PartyHost))
	{
		return false;
	}

	return PartyHost->KickMember(MemberId);
}

bool UNexusPartyManager::JoinParty(const FString& HostAddress, const FUniqueNetIdRepl& LocalPlayerId, const FString& DisplayName)
{
	if (IsInParty())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("[PartyManager] JoinParty: already in a party."));
		OnPartyJoinedEvent.Broadcast(ENexusPartyResult::AlreadyInParty, FNexusPartyState());
		return false;
	}

	if (HostAddress.IsEmpty() || !LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] JoinParty: invalid HostAddress or LocalPlayerId."));
		OnPartyJoinedEvent.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
		return false;
	}

	UWorld* World = GameInstance.IsValid() ? GameInstance->GetWorld() : nullptr;
	if (!World)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] JoinParty: World is null."));
		OnPartyJoinedEvent.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PartyClient = World->SpawnActor<ANexusPartyBeaconClient>(SpawnParams);

	if (!IsValid(PartyClient))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] JoinParty: failed to spawn ANexusPartyBeaconClient."));
		OnPartyJoinedEvent.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
		return false;
	}

	PartyClient->SetLocalPlayerId(LocalPlayerId);
	PartyClient->SetLocalDisplayName(DisplayName);

	BindPartyClientDelegates();

	if (!PartyClient->ConnectToHost(HostAddress))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] JoinParty: ConnectToHost failed."));
		CleanupPartyClient();
		OnPartyJoinedEvent.Broadcast(ENexusPartyResult::ConnectionFailed, FNexusPartyState());
		return false;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Connecting to party at '%s'..."), *HostAddress);
	// Result arrives asynchronously via OnClientJoinResult
	return true;
}

void UNexusPartyManager::LeaveParty()
{
	if (!IsInParty() || IsPartyLeader())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("[PartyManager] LeaveParty: not a non-leader member. Use DisbandParty() if you are the leader."));
		return;
	}

	if (IsValid(PartyClient))
	{
		// Notify the server gracefully before destroying the beacon
		PartyClient->ServerLeaveParty();
	}

	CleanupPartyClient();
	CachedPartyState = FNexusPartyState();

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Left the party."));
	OnPartyDisbandedEvent.Broadcast(ENexusPartyResult::Success);
}

bool UNexusPartyManager::IsInParty() const
{
	return CachedPartyState.IsValid();
}

bool UNexusPartyManager::IsPartyLeader() const
{
	return IsValid(PartyHost) && PartyHost->IsPartyActive();
}

bool UNexusPartyManager::GetLocalPlayerInfo(FUniqueNetIdRepl& OutId, FString& OutDisplayName) const
{
	if (!GameInstance.IsValid())
	{
		return false;
	}

	const ULocalPlayer* LocalPlayer = GameInstance->GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		return false;
	}

	const FUniqueNetIdRepl PlayerId = LocalPlayer->GetPreferredUniqueNetId();
	if (!PlayerId.IsValid())
	{
		return false;
	}

	OutId = PlayerId;

	// Prefer the OSS display name; fall back to the LocalPlayer nickname
	if (const IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld()))
	{
		const IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
		if (Identity.IsValid())
		{
			OutDisplayName = Identity->GetPlayerNickname(*PlayerId.GetUniqueNetId());
		}
	}

	if (OutDisplayName.IsEmpty())
	{
		OutDisplayName = LocalPlayer->GetNickname();
	}

	return true;
}

void UNexusPartyManager::BindPartyHostDelegates()
{
	if (!IsValid(PartyHost))
	{
		return;
	}

	UnbindPartyHostDelegates();

	HostMemberJoinedDelegateHandle = PartyHost->NativeOnMemberJoinedEvent.AddUObject(this, &UNexusPartyManager::OnHostMemberJoined);
	HostMemberLeftDelegateHandle = PartyHost->NativeOnMemberLeftEvent.AddUObject(this, &UNexusPartyManager::OnHostMemberLeft);
	HostStateChangedDelegateHandle = PartyHost->NativeOnStateChangedEvent.AddUObject(this, &UNexusPartyManager::OnHostStateChanged);
}

void UNexusPartyManager::UnbindPartyHostDelegates()
{
	if (IsValid(PartyHost))
	{
		PartyHost->NativeOnMemberJoinedEvent.Remove(HostMemberJoinedDelegateHandle);
		PartyHost->NativeOnMemberLeftEvent.Remove(HostMemberLeftDelegateHandle);
		PartyHost->NativeOnStateChangedEvent.Remove(HostStateChangedDelegateHandle);
	}

	HostMemberJoinedDelegateHandle.Reset();
	HostMemberLeftDelegateHandle.Reset();
	HostStateChangedDelegateHandle.Reset();
}

void UNexusPartyManager::BindPartyClientDelegates()
{
	if (!IsValid(PartyClient))
	{
		return;
	}

	ClientJoinResultDelegateHandle = PartyClient->NativeOnJoinResultEvent.AddUObject(this, &UNexusPartyManager::OnClientJoinResult);
	ClientStateUpdatedDelegateHandle = PartyClient->NativeOnStateUpdatedEvent.AddUObject(this, &UNexusPartyManager::OnClientStateUpdated);
	ClientKickedDelegateHandle = PartyClient->NativeOnKickedEvent.AddUObject(this, &UNexusPartyManager::OnClientKicked);
}

void UNexusPartyManager::CleanupPartyClient()
{
	if (IsValid(PartyClient))
	{
		PartyClient->NativeOnJoinResultEvent.Remove(ClientJoinResultDelegateHandle);
		PartyClient->NativeOnStateUpdatedEvent.Remove(ClientStateUpdatedDelegateHandle);
		PartyClient->NativeOnKickedEvent.Remove(ClientKickedDelegateHandle);

		PartyClient->DestroyBeacon();
		PartyClient = nullptr;
	}

	ClientJoinResultDelegateHandle.Reset();
	ClientStateUpdatedDelegateHandle.Reset();
	ClientKickedDelegateHandle.Reset();
}

void UNexusPartyManager::OnHostMemberJoined(const FUniqueNetIdRepl& MemberId, const FNexusPartyState& PartyState)
{
	CachedPartyState = PartyState;

	const FNexusPartySlot* NewSlot = PartyState.Members.FindByPredicate([&MemberId](const FNexusPartySlot& S)
	{
		return S.MemberId == MemberId;
	});

	if (NewSlot)
	{
		OnPartyMemberJoinedEvent.Broadcast(*NewSlot, PartyState);
	}

	OnPartyStateUpdatedEvent.Broadcast(PartyState);
}

void UNexusPartyManager::OnHostMemberLeft(const FUniqueNetIdRepl& MemberId, ENexusPartyMemberStatus MemberState)
{
	// Refresh cache from the authoritative source
	if (IsValid(PartyHost))
	{
		CachedPartyState = PartyHost->GetPartyState();
	}

	OnPartyMemberLeftEvent.Broadcast(MemberId, MemberState);
	OnPartyStateUpdatedEvent.Broadcast(CachedPartyState);
}

void UNexusPartyManager::OnHostStateChanged(const FNexusPartyState& PartyState)
{
	CachedPartyState = PartyState;
	OnPartyStateUpdatedEvent.Broadcast(PartyState);
}

void UNexusPartyManager::OnClientJoinResult(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState)
{
	if (PartyResult == ENexusPartyResult::Success)
	{
		CachedPartyState = PartyState;
		NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Joined party. Active: %d/%d."), PartyState.GetActiveCount(), PartyState.MaxSize);
	}
	else
	{
		// Join failed — release the beacon so subsequent attempts are allowed
		CleanupPartyClient();
		NEXUS_LOG(LogNexus, Warning, TEXT("[PartyManager] Failed to join party: %s."), *FString(LexToString(PartyResult)));
	}

	OnPartyJoinedEvent.Broadcast(PartyResult, PartyState);
}

void UNexusPartyManager::OnClientStateUpdated(const FNexusPartyState& PartyState)
{
	CachedPartyState = PartyState;
	OnPartyStateUpdatedEvent.Broadcast(PartyState);
}

void UNexusPartyManager::OnClientKicked()
{
	NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Kicked from party by leader."));

	CleanupPartyClient();
	CachedPartyState = FNexusPartyState();

	OnPartyMemberLeftEvent.Broadcast(FUniqueNetIdRepl(), ENexusPartyMemberStatus::Kicked);
	OnPartyDisbandedEvent.Broadcast(ENexusPartyResult::Success);
}
