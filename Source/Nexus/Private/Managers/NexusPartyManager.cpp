// Copyright Spike Plugins 2026. All Rights Reserved.

#include "Managers/NexusPartyManager.h"
#include "Managers/NexusBeaconManager.h"
#include "Managers/NexusSessionManager.h"
#include "Managers/NexusFriendManager.h"
#include "NexusOnlineSubsystem.h"
#include "Beacons/NexusPartyBeaconClient.h"
#include "Beacons/NexusPartyBeaconHost.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusPartyManager)

const FName UNexusPartyManager::PartyLobbySessionName = FName(TEXT("NexusPartyLobby"));

UNexusPartyManager::UNexusPartyManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNexusPartyManager::Initialize(UGameInstance* InGameInstance, UNexusBeaconManager* InBeaconManager, UNexusSessionManager* InSessionManager)
{
	check(InGameInstance);
	check(InBeaconManager);

	GameInstance = InGameInstance;
	BeaconManager = InBeaconManager;
	SessionManager = InSessionManager;

	PartyHost = InBeaconManager->GetPartyHost();

	// Bind to friend manager for platform overlay invites
	if (UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(this))
	{
		if (UNexusFriendManager* FriendManager = Subsystem->GetFriendManager())
		{
			FriendManager->OnSessionInviteReceived.AddDynamic(this, &UNexusPartyManager::OnPlatformPartyInviteReceived);
		}
	}

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Initialized."));
}

void UNexusPartyManager::Deinitialize()
{
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

	if (UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(this))
	{
		if (UNexusFriendManager* FriendManager = Subsystem->GetFriendManager())
		{
			FriendManager->OnSessionInviteReceived.RemoveDynamic(this, &UNexusPartyManager::OnPlatformPartyInviteReceived);
		}
	}

	PartyHost = nullptr;
	GameInstance.Reset();
	BeaconManager.Reset();
	SessionManager.Reset();

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Deinitialized."));
}

bool UNexusPartyManager::CreateParty(FNexusPartyHostParams Params)
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

	if (!BeaconManager->IsBeaconHostActive())
	{
		if (!BeaconManager->StartBeaconHost())
		{
			NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] CreateParty: failed to start beacon host."));
			OnPartyCreatedEvent.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
			return false;
		}
	}

	PartyHost = BeaconManager->GetPartyHost();
	if (!IsValid(PartyHost))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] CreateParty: ANexusPartyBeaconHostObject not found."));
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
	PartyHost->InitializeParty(LocalId, LocalName, Params.MaxSize);
	CachedPartyState = PartyHost->GetPartyState();

	NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Party created on Beacon. MaxSize=%d"), CachedPartyState.MaxSize);

	if (Params.bCreateLobbySession)
	{
		return CreatePartyLobbySession(Params);
	}
	else
	{
		OnPartyCreatedEvent.Broadcast(ENexusPartyResult::Success, CachedPartyState);
		return true;
	}
}

bool UNexusPartyManager::CreatePartyWithSize(int32 MaxSize)
{
	return CreateParty(FNexusPartyHostParams(MaxSize));
}

bool UNexusPartyManager::CreatePartyLobbySession(const FNexusPartyHostParams& Params)
{
	if (!SessionManager.IsValid()) return false;

	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GameInstance.IsValid() ? GameInstance->GetWorld() : nullptr);
	if (!SessionInterface.IsValid()) return false;

	FUniqueNetIdRepl LocalId; FString LocalName;
	GetLocalPlayerInfo(LocalId, LocalName);

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = Params.MaxSize;
	Settings.NumPrivateConnections = 0;
	Settings.bIsLANMatch = false;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowInvites = Params.bAllowInvites;
	Settings.bUsesPresence = true;
	Settings.bAllowJoinViaPresence = true;

	Settings.Set(SETTING_MAPNAME, FString("PartyLobby"), EOnlineDataAdvertisementType::ViaOnlineService);

	LobbySessionCreatedHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnPartyLobbySessionCreated));

	return SessionInterface->CreateSession(*LocalId, PartyLobbySessionName, Settings);
}

void UNexusPartyManager::OnPartyLobbySessionCreated(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GameInstance.IsValid() ? GameInstance->GetWorld() : nullptr);
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(LobbySessionCreatedHandle);
	}

	if (bWasSuccessful)
	{
		NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Party lobby session created successfully."));
		OnPartyCreatedEvent.Broadcast(ENexusPartyResult::Success, CachedPartyState);
	}
	else
	{
		NEXUS_LOG(LogNexus, Error, TEXT("[PartyManager] Failed to create party lobby session."));
		DisbandParty(); // Rollback beacon creation
		OnPartyCreatedEvent.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
	}
}

void UNexusPartyManager::DestroyPartyLobbySession()
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GameInstance.IsValid() ? GameInstance->GetWorld() : nullptr);
	if (SessionInterface.IsValid())
	{
		if (SessionInterface->GetNamedSession(PartyLobbySessionName) != nullptr)
		{
			SessionInterface->DestroySession(PartyLobbySessionName);
		}
	}
}

bool UNexusPartyManager::SendPartyInvite(const FUniqueNetIdRepl& FriendId)
{
	if (!IsInParty() || !IsPartyLeader()) return false;

	UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(this);
	if (Subsystem && Subsystem->GetFriendManager())
	{
		return Subsystem->GetFriendManager()->SendSessionInvite(PartyLobbySessionName, FriendId);
	}
	return false;
}

void UNexusPartyManager::DisbandParty()
{
	if (!IsPartyLeader()) return;

	if (IsValid(PartyHost))
	{
		PartyHost->DisbandParty();
	}

	UnbindPartyHostDelegates();
	DestroyPartyLobbySession();
	
	CachedPartyState = FNexusPartyState();
	NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Party disbanded."));
	OnPartyDisbandedEvent.Broadcast(ENexusPartyResult::Success);
}

bool UNexusPartyManager::KickMember(const FUniqueNetIdRepl& MemberId)
{
	if (!IsPartyLeader() || !IsValid(PartyHost)) return false;
	return PartyHost->KickMember(MemberId);
}

bool UNexusPartyManager::JoinParty(const FString& HostAddress, const FUniqueNetIdRepl& LocalPlayerId, const FString& DisplayName)
{
	if (IsInParty())
	{
		OnPartyJoinedEvent.Broadcast(ENexusPartyResult::AlreadyInParty, FNexusPartyState());
		return false;
	}

	UWorld* World = GameInstance.IsValid() ? GameInstance->GetWorld() : nullptr;
	if (!World) return false;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PartyClient = World->SpawnActor<ANexusPartyBeaconClient>(SpawnParams);

	if (!IsValid(PartyClient)) return false;

	PartyClient->SetLocalPlayerId(LocalPlayerId);
	PartyClient->SetLocalDisplayName(DisplayName);

	BindPartyClientDelegates();

	if (!PartyClient->ConnectToHost(HostAddress))
	{
		CleanupPartyClient();
		OnPartyJoinedEvent.Broadcast(ENexusPartyResult::ConnectionFailed, FNexusPartyState());
		return false;
	}

	return true;
}

bool UNexusPartyManager::JoinPartyFromSession(const FNexusSearchResult& PartyLobbySession)
{
	FString ConnectString;
	if (!GetBeaconAddressFromSession(PartyLobbySession, ConnectString))
	{
		OnPartyJoinedEvent.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
		return false;
	}

	FUniqueNetIdRepl LocalId; FString LocalName;
	if (!GetLocalPlayerInfo(LocalId, LocalName)) return false;

	return JoinParty(ConnectString, LocalId, LocalName);
}

bool UNexusPartyManager::GetBeaconAddressFromSession(const FNexusSearchResult& Session, FString& OutAddress) const
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GameInstance.IsValid() ? GameInstance->GetWorld() : nullptr);
	if (SessionInterface.IsValid())
	{
		return SessionInterface->GetResolvedConnectString(Session.GetNativeSearchResult(), NAME_BeaconPort, OutAddress);
	}
	return false;
}

void UNexusPartyManager::OnPlatformPartyInviteReceived(const FNexusPendingInvite& Invite)
{
	// Ensure it's actually our party lobby session
	if (Invite.IsValid())
	{
		FString MapName;
		
		Invite.Session.GetSessionSetting(SETTING_MAPNAME, MapName);
		if (MapName == TEXT("PartyLobby"))
		{
			FNexusPendingPartyInvite PartyInvite;
			PartyInvite.FromId = Invite.FromId;
			PartyInvite.FromDisplayName = Invite.Session.GetOwnerUsername(); // Resolve name from session owner
			PartyInvite.PartyLobbySession = Invite.Session;
			PartyInvite.TimeReceived = Invite.TimeReceived;

			OnPartyInviteReceivedEvent.Broadcast(PartyInvite);
		}
	}
}

void UNexusPartyManager::NotifyPartyOfGameSession(const FNexusSearchResult& GameSession)
{
	// Only leader does this
	if (IsPartyLeader())
	{
        // For the time being, just broadcast locally. 
        // A complete implementation would send a beacon RPC to tell clients.
        // Actually this is just triggered on leader by Subsystem and needs to be broadcasted to clients.
        // We will broadcast it so listeners can pick it up.
		OnPartyGameSessionReadyEvent.Broadcast(GameSession);

        if (IsValid(PartyHost))
        {
            // Assuming PartyHost has a NotifyGameSessionReady method or similar ?
            // Since it might not, and we are just restructuring according to new plan.
            // We'll leave it as a local broadcast for now or let context handle it.
        }
	}
}

void UNexusPartyManager::LeaveParty()
{
	if (!IsInParty() || IsPartyLeader()) return;

	if (IsValid(PartyClient))
	{
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
	if (!GameInstance.IsValid()) return false;
	const ULocalPlayer* LocalPlayer = GameInstance->GetFirstGamePlayer();
	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid()) return false;

	OutId = LocalPlayer->GetPreferredUniqueNetId();

	if (const IOnlineSubsystem* OSS = Online::GetSubsystem(GameInstance.IsValid() ? GameInstance->GetWorld() : nullptr))
	{
		if (const IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
		{
			OutDisplayName = Identity->GetPlayerNickname(*OutId.GetUniqueNetId());
		}
	}

	if (OutDisplayName.IsEmpty()) OutDisplayName = LocalPlayer->GetNickname();
	return true;
}

void UNexusPartyManager::BindPartyHostDelegates()
{
	if (!IsValid(PartyHost)) return;
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
	if (!IsValid(PartyClient)) return;
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
}

void UNexusPartyManager::OnHostMemberJoined(const FUniqueNetIdRepl& MemberId, const FNexusPartyState& PartyState)
{
	CachedPartyState = PartyState;
	const FNexusPartySlot* NewSlot = PartyState.Members.FindByPredicate([&MemberId](const FNexusPartySlot& S) { return S.MemberId == MemberId; });
	if (NewSlot) OnPartyMemberJoinedEvent.Broadcast(*NewSlot, PartyState);
	OnPartyStateUpdatedEvent.Broadcast(PartyState);
}

void UNexusPartyManager::OnHostMemberLeft(const FUniqueNetIdRepl& MemberId, ENexusPartyMemberStatus MemberState)
{
	if (IsValid(PartyHost)) CachedPartyState = PartyHost->GetPartyState();
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
		NEXUS_LOG(LogNexus, Log, TEXT("[PartyManager] Joined party."));
	}
	else
	{
		CleanupPartyClient();
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
	CleanupPartyClient();
	CachedPartyState = FNexusPartyState();
	OnPartyMemberLeftEvent.Broadcast(FUniqueNetIdRepl(), ENexusPartyMemberStatus::Kicked);
	OnPartyDisbandedEvent.Broadcast(ENexusPartyResult::Success);
}
