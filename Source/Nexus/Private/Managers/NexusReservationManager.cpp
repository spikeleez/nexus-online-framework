#include "Managers/NexusReservationManager.h"
#include "NexusOnlineSettings.h"
#include "NexusLog.h"
#include "PartyBeaconHost.h"
#include "PartyBeaconClient.h"
#include "PartyBeaconState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UNexusReservationManager::UNexusReservationManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ReservationHost(nullptr)
	, ActiveReservationClient(nullptr)
	, bReservationHostActive(false)
	, CachedSessionName(NAME_GameSession)
	, CachedMaxReservations(4)
	, CachedTeamCount(1)
	, CachedTeamSize(4)
{

}

UWorld* UNexusReservationManager::GetWorld() const
{
	return CachedGameInstance.IsValid() ? CachedGameInstance->GetWorld() : nullptr;
}

void UNexusReservationManager::Initialize(UGameInstance* InGameInstance)
{
	check(InGameInstance);
	CachedGameInstance = InGameInstance;
	NEXUS_LOG(LogNexus, Log, TEXT("ReservationManager initialized."));
}

void UNexusReservationManager::Deinitialize()
{
	StopReservationHost();

	// Clean up any in-flight client request.
	if (IsValid(ActiveReservationClient))
	{
		ActiveReservationClient->DestroyBeacon();
		ActiveReservationClient = nullptr;
	}

	CachedGameInstance.Reset();
	NEXUS_LOG(LogNexus, Log, TEXT("ReservationManager deinitialized."));
}

bool UNexusReservationManager::StartReservationHost(FName SessionName, int32 MaxReservations, int32 TeamCount /*= 1*/, int32 TeamSize /*= 0*/)
{
	if (bReservationHostActive)
	{
		NEXUS_LOG(LogNexus, Verbose, TEXT("ReservationHost already active skipping."));
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("World is null."));
		return false;
	}

	const ENetMode NetMode = World->GetNetMode();
	if (NetMode == NM_Client)
	{
		NEXUS_LOG(LogNexus, Log, TEXT("Skipped not a server."));
		return false;
	}

	if (MaxReservations <= 0)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("MaxReservations must be > 0."));
		return false;
	}

	// Resolve TeamSize: if caller passed 0, flatten to MaxReservations (single pool).
	const int32 ResolvedTeamSize = (TeamSize <= 0) ? MaxReservations : TeamSize;
	const int32 ResolvedTeamCount = FMath::Max(1, TeamCount);

	// Spawn APartyBeaconHost
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ReservationHost = World->SpawnActor<APartyBeaconHost>(SpawnParams);

	if (!ReservationHost)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to spawn APartyBeaconHost."));
		return false;
	}

	const UNexusOnlineSettings* Settings = UNexusOnlineSettings::Get();

	// InitHost sets up the UDP listener.
	if (!ReservationHost->InitHostBeacon(ResolvedTeamCount, ResolvedTeamSize, MaxReservations, SessionName))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("InitHost() failed on port %d."), Settings->ReservationListenPort);
		ReservationHost->Destroy();
		ReservationHost = nullptr;
		return false;
	}

	// Bind delegates
	ReservationHost->OnReservationsFull().BindUObject(this, &UNexusReservationManager::OnReservationsFull);
	ReservationHost->OnCancelationReceived().BindUObject(this, &UNexusReservationManager::OnCancelledReservation);
	ReservationHost->OnNewPlayerAdded().BindUObject(this, &UNexusReservationManager::OnNewPlayerAdded);
	ReservationHost->OnReservationChanged().BindUObject(this, &UNexusReservationManager::OnReservationChanged);

	// Allow connections.
	//ReservationHost->PauseBeacons(false);

	// Cache for potential re-use
	CachedSessionName = SessionName;
	CachedMaxReservations = MaxReservations;
	CachedTeamCount = ResolvedTeamCount;
	CachedTeamSize = ResolvedTeamSize;

	bReservationHostActive = true;
	NEXUS_LOG(LogNexus, Log, TEXT("ReservationHost started. Port=%d, Session='%s', MaxSlots=%d, Teams=%d, TeamSize=%d."), Settings->ReservationListenPort, *SessionName.ToString(), MaxReservations, ResolvedTeamCount, ResolvedTeamSize);
	return true;
}

void UNexusReservationManager::StopReservationHost()
{
	if (!bReservationHostActive)
	{
		return;
	}

	if (IsValid(ReservationHost))
	{
		// Unbind delegates before destroy to prevent stale callbacks.
		ReservationHost->OnReservationsFull().Unbind();
		ReservationHost->OnCancelationReceived().Unbind();
		ReservationHost->OnNewPlayerAdded().Unbind();
		ReservationHost->OnReservationChanged().Unbind();

		ReservationHost->Destroy();
		ReservationHost = nullptr;
	}

	bReservationHostActive = false;
	NEXUS_LOG(LogNexus, Log, TEXT("ReservationHost stopped."));
}

int32 UNexusReservationManager::GetNumRemainingReservations() const
{
	if (!bReservationHostActive || !IsValid(ReservationHost))
	{
		return 0;
	}

	UPartyBeaconState* State = ReservationHost->GetState();
	return State ? State->GetRemainingReservations() : 0;
}

int32 UNexusReservationManager::GetNumPlayersReserved() const
{
	if (!bReservationHostActive || !IsValid(ReservationHost))
	{
		return 0;
	}

	UPartyBeaconState* State = ReservationHost->GetState();
	return State ? State->GetNumConsumedReservations() : 0;
}

APartyBeaconClient* UNexusReservationManager::RequestReservation(const FNexusSearchResult& SearchResult, const FNexusPartyReservation& Reservation)
{
	if (!SearchResult.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("SearchResult is invalid."));
		return nullptr;
	}

	if (!Reservation.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Reservation has invalid party leader."));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("World is null."));
		return nullptr;
	}

	// Only one active request allowed. This protects against duplicate requests which
	// APartyBeaconHost correctly rejects with EPartyReservationResult::DuplicateReservation,
	// but we catch it early on the client to avoid spawning extra actors.
	if (IsValid(ActiveReservationClient))
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("A request is already in flight. Cancel it first."));
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APartyBeaconClient* Client = World->SpawnActor<APartyBeaconClient>(SpawnParams);

	if (!Client)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to spawn APartyBeaconClient."));
		return nullptr;
	}

	// Bind the response delegate before sending.
	Client->OnReservationRequestComplete().BindUObject(this, &UNexusReservationManager::OnReservationRequestComplete);

	const FPartyReservation NativeReservation = Reservation.ToNative();
	const bool bRequested = Client->RequestReservation(SearchResult.GetNativeSearchResult(), Reservation.PartyLeader.UniqueId, NativeReservation.PartyMembers);
	if (!bRequested)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("APartyBeaconClient::RequestReservation failed."));
		Client->DestroyBeacon();
		return nullptr;
	}

	ActiveReservationClient = Client;
	NEXUS_LOG(LogNexus, Log, TEXT("Sent request for party of %d (leader: %s)"), Reservation.GetPartySize(), *Reservation.PartyLeader.UniqueId.ToString());
	return Client;
}

APartyBeaconClient* UNexusReservationManager::RequestReservationDirect(const FString& HostAddress, const FString& SessionId, const FNexusPartyReservation& Reservation)
{
	if (HostAddress.IsEmpty() || SessionId.IsEmpty())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("HostAddress or SessionId is empty."));
		return nullptr;
	}

	if (!Reservation.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Reservation has invalid party leader."));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (IsValid(ActiveReservationClient))
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("A request is already in flight. Cancel it first."));
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APartyBeaconClient* Client = World->SpawnActor<APartyBeaconClient>(SpawnParams);

	if (!Client)
	{
		return nullptr;
	}

	Client->OnReservationRequestComplete().BindUObject(this, &UNexusReservationManager::OnReservationRequestComplete);

	const FPartyReservation NativeReservation = Reservation.ToNative();
	const bool bRequested = Client->RequestReservation(HostAddress, SessionId, Reservation.PartyLeader.UniqueId, NativeReservation.PartyMembers);
	if (!bRequested)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("RequestReservation failed."));
		Client->DestroyBeacon();
		return nullptr;
	}

	ActiveReservationClient = Client;
	return Client;
}

void UNexusReservationManager::CancelReservation(const FUniqueNetIdRepl& PartyLeaderId)
{
	if (!IsValid(ActiveReservationClient))
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("No active client request."));
		return;
	}

	if (!PartyLeaderId.IsValid())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("Invalid PartyLeaderId."));
		return;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Cancelling for leader %s."), *PartyLeaderId.ToString());
	ActiveReservationClient->CancelReservation();

	// The client self-destructs after cancellation. Clear our pointer.
	ActiveReservationClient = nullptr;
}

void UNexusReservationManager::OnSessionCreated(ENexusCreateSessionResult Result)
{
	if (Result != ENexusCreateSessionResult::Success)
	{
		return;
	}

	if (!UNexusOnlineSettings::Get()->bAutoStartReservationHost)
	{
		return;
	}

	const UNexusOnlineSettings* Settings = UNexusOnlineSettings::Get();
	StartReservationHost(Settings->DefaultGameSessionName, Settings->DefaultMaxPlayers, 1, Settings->DefaultMaxPlayers);
}

void UNexusReservationManager::OnSessionDestroyed(ENexusDestroySessionResult Result)
{
	StopReservationHost();
}

void UNexusReservationManager::OnReservationChanged()
{
	if (!IsValid(ReservationHost))
	{
		return;
	}

	const int32 Reserved = GetNumPlayersReserved();
	const int32 Remaining = GetNumRemainingReservations();
	NEXUS_LOG(LogNexus, Verbose, TEXT("%d reserved, %d remaining."), Reserved, Remaining);
}

void UNexusReservationManager::OnReservationsFull()
{
	NEXUS_LOG(LogNexus, Log, TEXT("All slots reserved. Ready to start the game."));
	NativeOnReservationsFull.Broadcast();
}

void UNexusReservationManager::OnNewPlayerAdded(const FPlayerReservation& NewPlayer)
{
	const FUniqueNetIdRepl LeaderId(NewPlayer.UniqueId.GetUniqueNetId());

	FNexusReservationSummary Summary = BuildSummaryForLeader(LeaderId);
	if (!Summary.IsValid())
	{
		// Fallback: build a minimal summary from the player data alone.
		Summary.PartyLeaderId = LeaderId;
		Summary.PartySize = 1;
		Summary.TeamNum = -1;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("New reservation accepted. Leader=%s, PartySize=%d."), *LeaderId.ToString(), Summary.PartySize);
	NativeOnReservationAccepted.Broadcast(Summary);
}

void UNexusReservationManager::OnCancelledReservation(const FUniqueNetId& PartyLeaderId)
{
	NEXUS_LOG(LogNexus, Log, TEXT("A reservation was cancelled."));
	NativeOnReservationCancelled.Broadcast(PartyLeaderId);
}

void UNexusReservationManager::OnReservationRequestComplete(EPartyReservationResult::Type NativeResult)
{
	const ENexusReservationResult Result = ConvertReservationResult(NativeResult);

	NEXUS_LOG(LogNexus, Log, TEXT("Reservation response received: %s."), *LexToString(Result));

	NativeOnReservationResponse.Broadcast(Result);

	// The client self-destructs after receiving a response (success or failure).
	// Clear our reference so new requests are allowed.
	ActiveReservationClient = nullptr;
}

ENexusReservationResult UNexusReservationManager::ConvertReservationResult(EPartyReservationResult::Type NativeResult)
{
	// EPartyReservationResult lives in PartyBeaconState.h
	switch (NativeResult)
	{
	case EPartyReservationResult::ReservationAccepted:
		return ENexusReservationResult::Success;
	case EPartyReservationResult::PartyLimitReached:
		return ENexusReservationResult::SessionFull;
	case EPartyReservationResult::IncorrectPlayerCount:
		return ENexusReservationResult::PartyTooLarge;
	case EPartyReservationResult::ReservationDuplicate:
		return ENexusReservationResult::ReservationDuplicate;
	case EPartyReservationResult::ReservationNotFound:
		return ENexusReservationResult::ReservationNotFound;
	case EPartyReservationResult::BadSessionId:
		return ENexusReservationResult::BadSessionId;
	case EPartyReservationResult::RequestTimedOut:
	default:
		return ENexusReservationResult::GeneralError;
	}
}

FNexusReservationSummary UNexusReservationManager::BuildSummaryForLeader(const FUniqueNetIdRepl& LeaderId) const
{
	FNexusReservationSummary Summary;
	if (!IsValid(ReservationHost) || !LeaderId.IsValid())
	{
		return Summary;
	}

	UPartyBeaconState* State = ReservationHost->GetState();
	if (!State)
	{
		return Summary;
	}

	// Scan the reservation list for a matching leader.
	for (FPartyReservation& Reservation : State->GetReservations())
	{
		if (Reservation.PartyLeader.IsValid() && *Reservation.PartyLeader == *LeaderId.GetUniqueNetId())
		{
			Summary.PartyLeaderId = LeaderId;
			Summary.PartySize = Reservation.PartyMembers.Num(); // includes leader in UE5
			Summary.TeamNum = Reservation.TeamNum;
			break;
		}
	}

	return Summary;
}