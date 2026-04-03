// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Managers/NexusSessionManager.h"
#include "NexusLog.h"
#include "NexusOnlineSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusSessionManager)

UNexusSessionManager::UNexusSessionManager()
	: CurrentSessionState(ENexusSessionState::NoSession)
	, ActiveSessionName(NAME_None)
	, bPendingAutoTravel(false)
	, PendingStartingLevel(FString())
{

}

void UNexusSessionManager::Initialize(UGameInstance* InGameInstance)
{
	check(InGameInstance);
	GameInstanceRef = InGameInstance;
	CurrentSessionState = ENexusSessionState::NoSession;

	if (GEngine)
	{
		OnNetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(this, &UNexusSessionManager::OnNetworkFailure);
	}

	if (const IOnlineSessionPtr& SessionInterface = GetSessionInterface())
	{
		OnSessionFailureDelegateHandle = SessionInterface->AddOnSessionFailureDelegate_Handle(FOnSessionFailureDelegate::CreateUObject(this, &UNexusSessionManager::OnSessionFailure));
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Initialized."));
}

void UNexusSessionManager::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(OnNetworkFailureDelegateHandle);
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		SessionInterface->ClearOnSessionFailureDelegate_Handle(OnSessionFailureDelegateHandle);
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(OnSessionUpdateCompleteDelegateHandle);
	}

	CurrentSearchSettings.Reset();
	CachedSearchResults.Empty();

	NEXUS_LOG(LogNexus, Log, TEXT("Deinitialized."));
}

bool UNexusSessionManager::CreateSession(const FName SessionName, const FNexuHostParams& HostParams)
{
	if (OnCreateSessionCompleteDelegateHandle.IsValid())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("A creation operation is already in progress. Ignoring the call."));
		SendCreateSessionResult(ENexusCreateSessionResult::Failure);
		return false;
	}

	if (CurrentSessionState == ENexusSessionState::Creating || CurrentSessionState == ENexusSessionState::Pending)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("The current state already has a session pending or in the process of being created."));
		SendCreateSessionResult(ENexusCreateSessionResult::AlreadyExists);
		return false;
	}

	if (!HostParams.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Invalid host params."));
		SendCreateSessionResult(ENexusCreateSessionResult::Failure);
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("No valid session interface."));
		SendCreateSessionResult(ENexusCreateSessionResult::NoOnlineSubsystem);
		return false;
	}

	// Check if a session with this name already exists.
	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession != nullptr)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("Session '%s' already exists. Destroy it first."), *SessionName.ToString());
		SendCreateSessionResult(ENexusCreateSessionResult::AlreadyExists);
		return false;
	}

	// Get local player ID.
	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Could not get local player unique ID."));
		SendCreateSessionResult(ENexusCreateSessionResult::Failure);
		return false;
	}

	// Build session settings from host params.
	FOnlineSessionSettings NewSettings;
	HostParams.ToOnlineSessionSettings(NewSettings);

	ActiveSessionName = SessionName;
	PendingStartingLevel = HostParams.StartingLevel;

	// Bind completion delegate.
	OnCreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnSessionCreationComplete));

	SetSessionState(SessionName, ENexusSessionState::Creating);

	NEXUS_LOG(LogNexus, Log, TEXT("CreateSession: Creating session '%s' (MaxPlayers: %d, Presence: %s, StartingLevel: '%s')..."),
		*SessionName.ToString(), HostParams.MaxNumPlayers,
		HostParams.bUsesPresence ? TEXT("true") : TEXT("false"),
		PendingStartingLevel.IsEmpty() ? TEXT("none") : *PendingStartingLevel);

	if (!SessionInterface->CreateSession(*LocalPlayerId, SessionName, NewSettings))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Platform CreateSession call failed."));
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		SetSessionState(SessionName, ENexusSessionState::NoSession);
		PendingStartingLevel.Empty();
		SendCreateSessionResult(ENexusCreateSessionResult::Failure);
		return false;
	}

	return true;
}

bool UNexusSessionManager::UpdateSession(const FName SessionName, const FNexuHostParams& NewHostParams)
{
	if (OnSessionUpdateCompleteDelegateHandle.IsValid())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("A session update already running, Please wait for its completion."));
		SendUpdateSessionResult(ENexusUpdateSessionResult::Failure);
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("No valid session interface."));
		SendUpdateSessionResult(ENexusUpdateSessionResult::NoOnlineSubsystem);
		return false;
	}

	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession == nullptr)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("Session '%s' not found to update."), *SessionName.ToString());
		SendUpdateSessionResult(ENexusUpdateSessionResult::NoSession);
		return false;
	}

	FOnlineSessionSettings UpdatedSettings = ExistingSession->SessionSettings;
	NewHostParams.ToOnlineSessionSettings(UpdatedSettings);

	OnSessionUpdateCompleteDelegateHandle = SessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UNexusSessionManager::OnSessionUpdatedComplete));

	NEXUS_LOG(LogNexus, Log, TEXT("Updating session '%s'..."), *SessionName.ToString());

	if (!SessionInterface->UpdateSession(SessionName, UpdatedSettings, true))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to Update session with Native API."));
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(OnSessionUpdateCompleteDelegateHandle);
		OnSessionUpdateCompleteDelegateHandle.Reset();
		SendUpdateSessionResult(ENexusUpdateSessionResult::Failure);
		return false;
	}

	return true;
}

bool UNexusSessionManager::FindSessions(const FNexusSearchParams& SearchParams)
{
	if (OnFindSessionsCompleteDelegateHandle.IsValid())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("A search is already underway. Please wait for its completion."));
		SendFindSessionResult(ENexusFindSessionsResult::Failure, TArray<FNexusSearchResult>());
		return false;
	}

	if (!SearchParams.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Invalid search params."));
		SendFindSessionResult(ENexusFindSessionsResult::Failure, TArray<FNexusSearchResult>());
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("No valid session interface."));
		SendFindSessionResult(ENexusFindSessionsResult::NoOnlineSubsystem, TArray<FNexusSearchResult>());
		return false;
	}

	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Could not get local player unique ID."));
		SendFindSessionResult(ENexusFindSessionsResult::Failure, TArray<FNexusSearchResult>());
		return false;
	}

	// Build the online search object.
	CurrentSearchSettings = MakeShareable(new FOnlineSessionSearch());
	CurrentSearchSettings->MaxSearchResults = SearchParams.MaxSearchResults;
	CurrentSearchSettings->bIsLanQuery = SearchParams.bIsLanQuery;

	// Enable presence search if requested (required for Steam/EOS lobbies).
	if (SearchParams.bSearchPresence)
	{
		CurrentSearchSettings->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}

	// Apply extra query settings for server-side filtering.
	for (const FNexusQuerySetting& QuerySetting : SearchParams.ExtraQuerySettings)
	{
		if (QuerySetting.IsValid())
		{
			CurrentSearchSettings->QuerySettings.SearchParams.Add(QuerySetting.Key, FOnlineSessionSearchParam(QuerySetting.Data, EOnlineComparisonOp::Equals));
		}
	}

	// Bind completion delegate.
	OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UNexusSessionManager::OnSessionFoundComplete));

	NEXUS_LOG(LogNexus, Log, TEXT("Searching (MaxResults: %d, LAN: %s, Presence: %s)..."),
		SearchParams.MaxSearchResults,
		SearchParams.bIsLanQuery ? TEXT("true") : TEXT("false"),
		SearchParams.bSearchPresence ? TEXT("true") : TEXT("false"));

	if (!SessionInterface->FindSessions(*LocalPlayerId, CurrentSearchSettings.ToSharedRef()))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Platform FindSessions call failed."));
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		SendFindSessionResult(ENexusFindSessionsResult::Failure, TArray<FNexusSearchResult>());
		return false;
	}

	return true;
}

bool UNexusSessionManager::JoinSession(const FName SessionName, const FNexusSearchResult& SearchResult, const bool bAutoTravel /*= true*/)
{
	if (OnJoinSessionCompleteDelegateHandle.IsValid())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("A join session request already pending. Please wait for its completion."));
		SendJoinSessionResult(ENexusJoinSessionResult::Failure);
		return false;
	}

	if (!SearchResult.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Invalid search result."));
		SendJoinSessionResult(ENexusJoinSessionResult::Failure);
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("No valid session interface."));
		SendJoinSessionResult(ENexusJoinSessionResult::NoOnlineSubsystem);
		return false;
	}

	// Prevent joining if already in a session with this name.
	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession != nullptr)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("Already in session '%s'. Destroy it first."), *SessionName.ToString());
		SendJoinSessionResult(ENexusJoinSessionResult::AlreadyInSession);
		return false;
	}

	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Could not get local player unique ID."));
		SendJoinSessionResult(ENexusJoinSessionResult::Failure);
		return false;
	}

	ActiveSessionName = SessionName;
	bPendingAutoTravel = bAutoTravel;

	// Bind completion delegate.
	OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UNexusSessionManager::OnSessionJoinedComplete));

	NEXUS_LOG(LogNexus, Log, TEXT("Joining session '%s' (Host: %s, AutoTravel: %s)..."), *SessionName.ToString(), *SearchResult.GetOwnerUsername(), bAutoTravel ? TEXT("true") : TEXT("false"));

	if (!SessionInterface->JoinSession(*LocalPlayerId, SessionName, SearchResult.OnlineResult))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Platform JoinSession call failed."));
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		bPendingAutoTravel = false;
		SendJoinSessionResult(ENexusJoinSessionResult::Failure);
		return false;
	}

	return true;
}

bool UNexusSessionManager::DestroySession(const FName SessionName)
{
	if (OnDestroySessionCompleteDelegateHandle.IsValid())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("A destroy session request already pending. Please wait for its completion."));
		SendDestroySessionResult(ENexusDestroySessionResult::Failure);
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("DestroySession: No valid session interface."));
		SendDestroySessionResult(ENexusDestroySessionResult::Failure);
		return false;
	}

	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName.IsNone() ? NAME_GameSession : SessionName);
	if (ExistingSession == nullptr)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("DestroySession: No session '%s' to destroy."), *SessionName.ToString());
		SendDestroySessionResult(ENexusDestroySessionResult::NoSession);
		return false;
	}

	ActiveSessionName = SessionName;
	SetSessionState(SessionName, ENexusSessionState::Destroying);

	// Bind completion delegate.
	OnDestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UNexusSessionManager::OnSessionDestroyedComplete));

	NEXUS_LOG(LogNexus, Log, TEXT("DestroySession: Destroying session '%s'..."), *SessionName.ToString());

	if (!SessionInterface->DestroySession(SessionName))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("DestroySession: Platform DestroySession call failed."));
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		SetSessionState(SessionName, ENexusSessionState::NoSession);
		SendDestroySessionResult(ENexusDestroySessionResult::Failure);
		return false;
	}

	return true;
}

bool UNexusSessionManager::IsInSession() const
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	const UNexusOnlineSettings* Settings = UNexusOnlineSettings::Get();
	return SessionInterface->GetNamedSession(Settings->DefaultGameSessionName) != nullptr;
}

FNexusNamedSession UNexusSessionManager::GetNamedSession(const FName SessionName) const
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return FNexusNamedSession();
	}

	const FNamedOnlineSession* Session = SessionInterface->GetNamedSession(SessionName);
	return FNexusNamedSession(Session);
}

bool UNexusSessionManager::GetSessionConnectString(const FName SessionName, FString& OutConnectString) const
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	return SessionInterface->GetResolvedConnectString(SessionName, OutConnectString);
}

void UNexusSessionManager::SetSessionState(const FName SessionName, const ENexusSessionState NewState)
{
	if (CurrentSessionState != NewState)
	{
		NEXUS_LOG(LogNexus, Log, TEXT("%s -> %s (Session: %s)"), LexToString(CurrentSessionState), LexToString(NewState), *SessionName.ToString());

		CurrentSessionState = NewState;
		OnSessionStateChangedEvent.Broadcast(SessionName, NewState);
	}
}

IOnlineSessionPtr UNexusSessionManager::GetSessionInterface() const
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	return OnlineSub ? OnlineSub->GetSessionInterface() : nullptr;
}

FUniqueNetIdPtr UNexusSessionManager::GetLocalPlayerId() const
{
	if (!GameInstanceRef.IsValid())
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = GameInstanceRef->GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		return nullptr;
	}

	return LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
}

void UNexusSessionManager::SendCreateSessionResult(const ENexusCreateSessionResult Result)
{
	OnSessionCreatedEvent.Broadcast(Result);
	NativeOnSessionCreated.Broadcast(Result);
}

void UNexusSessionManager::SendFindSessionResult(const ENexusFindSessionsResult Result, const TArray<FNexusSearchResult>& Results)
{
	OnSessionsFoundEvent.Broadcast(Result, Results);
	NativeOnSessionsFound.Broadcast(Result, Results);
}

void UNexusSessionManager::SendJoinSessionResult(const ENexusJoinSessionResult Result)
{
	OnSessionJoinedEvent.Broadcast(Result);
	NativeOnSessionJoined.Broadcast(Result);
}

void UNexusSessionManager::SendDestroySessionResult(const ENexusDestroySessionResult Result)
{
	OnSessionDestroyedEvent.Broadcast(Result);
	NativeOnSessionDestroyed.Broadcast(Result);
}

void UNexusSessionManager::SendUpdateSessionResult(const ENexusUpdateSessionResult Result)
{
	OnSessionUpdatedEvent.Broadcast(Result);
	NativeOnSessionUpdated.Broadcast(Result);
}

void UNexusSessionManager::OnSessionCreationComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	}

	OnCreateSessionCompleteDelegateHandle.Reset();

	if (!bWasSuccessful)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to create session '%s'."), *SessionName.ToString());

		SetSessionState(SessionName, ENexusSessionState::NoSession);
		PendingStartingLevel.Empty();

		SendCreateSessionResult(ENexusCreateSessionResult::Failure);
		return;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Session '%s' created successfully."), *SessionName.ToString());
	SetSessionState(SessionName, ENexusSessionState::Pending);

	// Broadcast before travel so listeners can react (show loading screen, etc).
	SendCreateSessionResult(ENexusCreateSessionResult::Success);

	// Auto-travel to starting level if one was specified.
	if (!PendingStartingLevel.IsEmpty())
	{
		TravelToStartingLevel();
	}
}

void UNexusSessionManager::TravelToStartingLevel()
{
	if (!GameInstanceRef.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("GameInstance is invalid."));
		return;
	}

	UWorld* World = GameInstanceRef->GetWorld();
	if (!World)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("World is invalid."));
		return;
	}

	// Append ?listen if not already present the host must be a listen server.
	FString TravelURL = PendingStartingLevel;
	if (!TravelURL.Contains(TEXT("listen")))
	{
		TravelURL.Append(TEXT("?listen"));
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Traveling to '%s'..."), *TravelURL);
	GEngine->SetClientTravel(World, *TravelURL, TRAVEL_Absolute);
	PendingStartingLevel.Empty();
}

void UNexusSessionManager::OnSessionFoundComplete(bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
	}

	CachedSearchResults.Empty();

	if (bWasSuccessful && CurrentSearchSettings.IsValid())
	{
		const TArray<FOnlineSessionSearchResult>& Results = CurrentSearchSettings->SearchResults;
		NEXUS_LOG(LogNexus, Log, TEXT("Found %d session(s)."), Results.Num());

		CachedSearchResults.Reserve(Results.Num());
		for (const FOnlineSessionSearchResult& Result : Results)
		{
			CachedSearchResults.Emplace(FNexusSearchResult(Result));
		}

		const ENexusFindSessionsResult FinalResult = CachedSearchResults.Num() > 0
			? ENexusFindSessionsResult::Success
			: ENexusFindSessionsResult::NoResults;

		SendFindSessionResult(FinalResult, CachedSearchResults);
	}
	else
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Search failed."));
		SendFindSessionResult(ENexusFindSessionsResult::Failure, CachedSearchResults);
	}
}

void UNexusSessionManager::OnSessionJoinedComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
	}

	ENexusJoinSessionResult FinalResult;

	switch (Result)
	{
	case EOnJoinSessionCompleteResult::Success:
		NEXUS_LOG(LogNexus, Log, TEXT("Successfully joined session '%s'."), *SessionName.ToString());
		SetSessionState(SessionName, ENexusSessionState::Pending);
		FinalResult = ENexusJoinSessionResult::Success;
		break;

	case EOnJoinSessionCompleteResult::SessionIsFull:
		NEXUS_LOG(LogNexus, Warning, TEXT("Session '%s' is full."), *SessionName.ToString());
		FinalResult = ENexusJoinSessionResult::SessionFull;
		break;

	case EOnJoinSessionCompleteResult::SessionDoesNotExist:
		NEXUS_LOG(LogNexus, Warning, TEXT("Session '%s' does not exist."), *SessionName.ToString());
		FinalResult = ENexusJoinSessionResult::SessionNotFound;
		break;

	case EOnJoinSessionCompleteResult::AlreadyInSession:
		NEXUS_LOG(LogNexus, Warning, TEXT("Already in session '%s'."), *SessionName.ToString());
		FinalResult = ENexusJoinSessionResult::AlreadyInSession;
		break;

	default:
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to join session '%s' (Result: %d)."), *SessionName.ToString(), static_cast<int32>(Result));
		FinalResult = ENexusJoinSessionResult::Failure;
		break;
	}

	// Broadcast before travel so listeners can react (show loading screen, etc).
	SendJoinSessionResult(FinalResult);

	// Auto-travel to the host if join was successful and auto-travel was requested.
	if (FinalResult == ENexusJoinSessionResult::Success && bPendingAutoTravel)
	{
		TravelToSession(SessionName);
	}

	bPendingAutoTravel = false;
}

void UNexusSessionManager::TravelToSession(const FName SessionName)
{
	FString ConnectString;
	if (!GetSessionConnectString(SessionName, ConnectString) || ConnectString.IsEmpty())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to get connect string for session '%s'."), *SessionName.ToString());
		return;
	}

	if (!GameInstanceRef.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("GameInstance is invalid."));
		return;
	}

	UWorld* World = GameInstanceRef->GetWorld();
	if (!World)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("World is invalid."));
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("No PlayerController found."));
		return;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Traveling to '%s'..."), *ConnectString);
	PC->ClientTravel(ConnectString, TRAVEL_Absolute);
}

void UNexusSessionManager::OnSessionDestroyedComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
	}

	SetSessionState(SessionName, ENexusSessionState::NoSession);

	if (bWasSuccessful)
	{
		NEXUS_LOG(LogNexus, Log, TEXT("Session '%s' destroyed successfully."), *SessionName.ToString());
		SendDestroySessionResult(ENexusDestroySessionResult::Success);
	}
	else
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to destroy session '%s'."), *SessionName.ToString());
		SendDestroySessionResult(ENexusDestroySessionResult::Failure);
	}
}

void UNexusSessionManager::OnSessionUpdatedComplete(FName SessionName, bool bWasSuccessfully)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(OnSessionUpdateCompleteDelegateHandle);
	}

	OnSessionUpdateCompleteDelegateHandle.Reset();

	if (bWasSuccessfully)
	{
		NEXUS_LOG(LogNexus, Log, TEXT("Session '%s' successfully updated."), *SessionName.ToString());
		SendUpdateSessionResult(ENexusUpdateSessionResult::Success);
	}
	else
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to update session '%s'."), *SessionName.ToString());
		SendUpdateSessionResult(ENexusUpdateSessionResult::Failure);
	}
}

void UNexusSessionManager::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	NEXUS_LOG(LogNexus, Error, TEXT("Network Error detected: %s"), *ErrorString);

	SetSessionState(ActiveSessionName, ENexusSessionState::NoSession);

	PendingStartingLevel.Empty();
	bPendingAutoTravel = false;

	OnNetworkErrorEvent.Broadcast(ErrorString);
}

void UNexusSessionManager::OnSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType)
{
	NEXUS_LOG(LogNexus, Error, TEXT("Session Error detected: %d"), static_cast<int32>(FailureType));

	SetSessionState(ActiveSessionName, ENexusSessionState::NoSession);

	PendingStartingLevel.Empty();
	bPendingAutoTravel = false;

	FString ErrorMessage = TEXT("The online session was lost or the host disconnected.");
	OnNetworkErrorEvent.Broadcast(ErrorMessage);
}
