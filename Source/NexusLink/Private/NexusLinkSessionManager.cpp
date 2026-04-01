// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusLinkSessionManager.h"
#include "NexusLog.h"
#include "NexusLinkSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

UNexusLinkSessionManager::UNexusLinkSessionManager()
	: CurrentSessionState(ENexusLinkSessionState::NoSession)
	, ActiveSessionName(NAME_None)
	, bPendingAutoTravel(false)
	, PendingStartingLevel(FString())
{

}

void UNexusLinkSessionManager::Initialize(UGameInstance* InGameInstance)
{
	check(InGameInstance);
	GameInstanceRef = InGameInstance;
	CurrentSessionState = ENexusLinkSessionState::NoSession;

	if (GEngine)
	{
		OnNetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(this, &UNexusLinkSessionManager::OnNetworkFailure);
	}

	if (const IOnlineSessionPtr& SessionInterface = GetSessionInterface())
	{
		OnSessionFailureDelegateHandle = SessionInterface->AddOnSessionFailureDelegate_Handle(FOnSessionFailureDelegate::CreateUObject(this, &UNexusLinkSessionManager::OnSessionFailure));
	}

	NEXUS_LOG(LogNexusLink, Log, TEXT("Initialized."));
}

void UNexusLinkSessionManager::Deinitialize()
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

	NEXUS_LOG(LogNexusLink, Log, TEXT("Deinitialized."));
}

bool UNexusLinkSessionManager::CreateSession(const FName SessionName, const FNexusLinkHostParams& HostParams)
{
	if (OnCreateSessionCompleteDelegateHandle.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("A creation operation is already in progress. Ignoring the call."));
		SendCreateSessionResult(ENexusLinkCreateSessionResult::Failure);
		return false;
	}

	if (CurrentSessionState == ENexusLinkSessionState::Creating || CurrentSessionState == ENexusLinkSessionState::Pending)
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("The current state already has a session pending or in the process of being created."));
		SendCreateSessionResult(ENexusLinkCreateSessionResult::AlreadyExists);
		return false;
	}

	if (!HostParams.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Invalid host params."));
		SendCreateSessionResult(ENexusLinkCreateSessionResult::Failure);
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No valid session interface."));
		SendCreateSessionResult(ENexusLinkCreateSessionResult::NoOnlineSubsystem);
		return false;
	}

	// Check if a session with this name already exists.
	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession != nullptr)
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("Session '%s' already exists. Destroy it first."), *SessionName.ToString());
		SendCreateSessionResult(ENexusLinkCreateSessionResult::AlreadyExists);
		return false;
	}

	// Get local player ID.
	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Could not get local player unique ID."));
		SendCreateSessionResult(ENexusLinkCreateSessionResult::Failure);
		return false;
	}

	// Build session settings from host params.
	FOnlineSessionSettings NewSettings;
	HostParams.ToOnlineSessionSettings(NewSettings);

	ActiveSessionName = SessionName;
	PendingStartingLevel = HostParams.StartingLevel;

	// Bind completion delegate.
	OnCreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnSessionCreationComplete));

	SetSessionState(SessionName, ENexusLinkSessionState::Creating);

	NEXUS_LOG(LogNexusLink, Log, TEXT("CreateSession: Creating session '%s' (MaxPlayers: %d, Presence: %s, StartingLevel: '%s')..."),
		*SessionName.ToString(), HostParams.MaxNumPlayers,
		HostParams.bUsesPresence ? TEXT("true") : TEXT("false"),
		PendingStartingLevel.IsEmpty() ? TEXT("none") : *PendingStartingLevel);

	if (!SessionInterface->CreateSession(*LocalPlayerId, SessionName, NewSettings))
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Platform CreateSession call failed."));
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		SetSessionState(SessionName, ENexusLinkSessionState::NoSession);
		PendingStartingLevel.Empty();
		SendCreateSessionResult(ENexusLinkCreateSessionResult::Failure);
		return false;
	}

	return true;
}

bool UNexusLinkSessionManager::UpdateSession(const FName SessionName, const FNexusLinkHostParams& NewHostParams)
{
	if (OnSessionUpdateCompleteDelegateHandle.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("A session update already running, Please wait for its completion."));
		SendUpdateSessionResult(ENexusLinkUpdateSessionResult::Failure);
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No valid session interface."));
		SendUpdateSessionResult(ENexusLinkUpdateSessionResult::NoOnlineSubsystem);
		return false;
	}

	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession == nullptr)
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("Session '%s' not found to update."), *SessionName.ToString());
		SendUpdateSessionResult(ENexusLinkUpdateSessionResult::NoSession);
		return false;
	}

	FOnlineSessionSettings UpdatedSettings = ExistingSession->SessionSettings;
	NewHostParams.ToOnlineSessionSettings(UpdatedSettings);

	OnSessionUpdateCompleteDelegateHandle = SessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UNexusLinkSessionManager::OnSessionUpdatedComplete));

	NEXUS_LOG(LogNexusLink, Log, TEXT("Updating session '%s'..."), *SessionName.ToString());

	if (!SessionInterface->UpdateSession(SessionName, UpdatedSettings, true))
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Failed to Update session with Native API."));
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(OnSessionUpdateCompleteDelegateHandle);
		OnSessionUpdateCompleteDelegateHandle.Reset();
		SendUpdateSessionResult(ENexusLinkUpdateSessionResult::Failure);
		return false;
	}

	return true;
}

bool UNexusLinkSessionManager::FindSessions(const FNexusLinkSearchParams& SearchParams)
{
	if (OnFindSessionsCompleteDelegateHandle.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("A search is already underway. Please wait for its completion."));
		SendFindSessionResult(ENexusLinkFindSessionsResult::Failure, TArray<FNexusLinkSearchResult>());
		return false;
	}

	if (!SearchParams.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Invalid search params."));
		SendFindSessionResult(ENexusLinkFindSessionsResult::Failure, TArray<FNexusLinkSearchResult>());
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No valid session interface."));
		SendFindSessionResult(ENexusLinkFindSessionsResult::NoOnlineSubsystem, TArray<FNexusLinkSearchResult>());
		return false;
	}

	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Could not get local player unique ID."));
		SendFindSessionResult(ENexusLinkFindSessionsResult::Failure, TArray<FNexusLinkSearchResult>());
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
	for (const FNexusLinkQuerySetting& QuerySetting : SearchParams.ExtraQuerySettings)
	{
		if (QuerySetting.IsValid())
		{
			CurrentSearchSettings->QuerySettings.SearchParams.Add(QuerySetting.Key, FOnlineSessionSearchParam(QuerySetting.Data, EOnlineComparisonOp::Equals));
		}
	}

	// Bind completion delegate.
	OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UNexusLinkSessionManager::OnSessionFoundComplete));

	NEXUS_LOG(LogNexusLink, Log, TEXT("Searching (MaxResults: %d, LAN: %s, Presence: %s)..."),
		SearchParams.MaxSearchResults,
		SearchParams.bIsLanQuery ? TEXT("true") : TEXT("false"),
		SearchParams.bSearchPresence ? TEXT("true") : TEXT("false"));

	if (!SessionInterface->FindSessions(*LocalPlayerId, CurrentSearchSettings.ToSharedRef()))
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Platform FindSessions call failed."));
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		SendFindSessionResult(ENexusLinkFindSessionsResult::Failure, TArray<FNexusLinkSearchResult>());
		return false;
	}

	return true;
}

bool UNexusLinkSessionManager::JoinSession(const FName SessionName, const FNexusLinkSearchResult& SearchResult, const bool bAutoTravel /*= true*/)
{
	if (OnJoinSessionCompleteDelegateHandle.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("A join session request already pending. Please wait for its completion."));
		SendJoinSessionResult(ENexusLinkJoinSessionResult::Failure);
		return false;
	}

	if (!SearchResult.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Invalid search result."));
		SendJoinSessionResult(ENexusLinkJoinSessionResult::Failure);
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No valid session interface."));
		SendJoinSessionResult(ENexusLinkJoinSessionResult::NoOnlineSubsystem);
		return false;
	}

	// Prevent joining if already in a session with this name.
	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession != nullptr)
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("Already in session '%s'. Destroy it first."), *SessionName.ToString());
		SendJoinSessionResult(ENexusLinkJoinSessionResult::AlreadyInSession);
		return false;
	}

	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Could not get local player unique ID."));
		SendJoinSessionResult(ENexusLinkJoinSessionResult::Failure);
		return false;
	}

	ActiveSessionName = SessionName;
	bPendingAutoTravel = bAutoTravel;

	// Bind completion delegate.
	OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UNexusLinkSessionManager::OnSessionJoinedComplete));

	NEXUS_LOG(LogNexusLink, Log, TEXT("Joining session '%s' (Host: %s, AutoTravel: %s)..."), *SessionName.ToString(), *SearchResult.GetOwnerUsername(), bAutoTravel ? TEXT("true") : TEXT("false"));

	if (!SessionInterface->JoinSession(*LocalPlayerId, SessionName, SearchResult.OnlineResult))
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Platform JoinSession call failed."));
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		bPendingAutoTravel = false;
		SendJoinSessionResult(ENexusLinkJoinSessionResult::Failure);
		return false;
	}

	return true;
}

bool UNexusLinkSessionManager::DestroySession(const FName SessionName)
{
	if (OnDestroySessionCompleteDelegateHandle.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("A destroy session request already pending. Please wait for its completion."));
		SendDestroySessionResult(ENexusLinkDestroySessionResult::Failure);
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("DestroySession: No valid session interface."));
		SendDestroySessionResult(ENexusLinkDestroySessionResult::Failure);
		return false;
	}

	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName.IsNone() ? NAME_GameSession : SessionName);
	if (ExistingSession == nullptr)
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("DestroySession: No session '%s' to destroy."), *SessionName.ToString());
		SendDestroySessionResult(ENexusLinkDestroySessionResult::NoSession);
		return false;
	}

	ActiveSessionName = SessionName;
	SetSessionState(SessionName, ENexusLinkSessionState::Destroying);

	// Bind completion delegate.
	OnDestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UNexusLinkSessionManager::OnSessionDestroyedComplete));

	NEXUS_LOG(LogNexusLink, Log, TEXT("DestroySession: Destroying session '%s'..."), *SessionName.ToString());

	if (!SessionInterface->DestroySession(SessionName))
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("DestroySession: Platform DestroySession call failed."));
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		SetSessionState(SessionName, ENexusLinkSessionState::NoSession);
		SendDestroySessionResult(ENexusLinkDestroySessionResult::Failure);
		return false;
	}

	return true;
}

bool UNexusLinkSessionManager::IsInSession() const
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	const UNexusLinkSettings* Settings = UNexusLinkSettings::Get();
	return SessionInterface->GetNamedSession(Settings->DefaultGameSessionName) != nullptr;
}

FNexusLinkNamedSession UNexusLinkSessionManager::GetNamedSession(const FName SessionName) const
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return FNexusLinkNamedSession();
	}

	const FNamedOnlineSession* Session = SessionInterface->GetNamedSession(SessionName);
	return FNexusLinkNamedSession(Session);
}

bool UNexusLinkSessionManager::GetSessionConnectString(const FName SessionName, FString& OutConnectString) const
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	return SessionInterface->GetResolvedConnectString(SessionName, OutConnectString);
}

void UNexusLinkSessionManager::SetSessionState(const FName SessionName, const ENexusLinkSessionState NewState)
{
	if (CurrentSessionState != NewState)
	{
		NEXUS_LOG(LogNexusLink, Log, TEXT("%s -> %s (Session: %s)"), LexToString(CurrentSessionState), LexToString(NewState), *SessionName.ToString());

		CurrentSessionState = NewState;
		OnSessionStateChangedEvent.Broadcast(SessionName, NewState);
	}
}

IOnlineSessionPtr UNexusLinkSessionManager::GetSessionInterface() const
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	return OnlineSub ? OnlineSub->GetSessionInterface() : nullptr;
}

FUniqueNetIdPtr UNexusLinkSessionManager::GetLocalPlayerId() const
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

void UNexusLinkSessionManager::SendCreateSessionResult(const ENexusLinkCreateSessionResult Result)
{
	OnSessionCreatedEvent.Broadcast(Result);
	NativeOnSessionCreated.Broadcast(Result);
}

void UNexusLinkSessionManager::SendFindSessionResult(const ENexusLinkFindSessionsResult Result, const TArray<FNexusLinkSearchResult>& Results)
{
	OnSessionsFoundEvent.Broadcast(Result, Results);
	NativeOnSessionsFound.Broadcast(Result, Results);
}

void UNexusLinkSessionManager::SendJoinSessionResult(const ENexusLinkJoinSessionResult Result)
{
	OnSessionJoinedEvent.Broadcast(Result);
	NativeOnSessionJoined.Broadcast(Result);
}

void UNexusLinkSessionManager::SendDestroySessionResult(const ENexusLinkDestroySessionResult Result)
{
	OnSessionDestroyedEvent.Broadcast(Result);
	NativeOnSessionDestroyed.Broadcast(Result);
}

void UNexusLinkSessionManager::SendUpdateSessionResult(const ENexusLinkUpdateSessionResult Result)
{
	OnSessionUpdatedEvent.Broadcast(Result);
	NativeOnSessionUpdated.Broadcast(Result);
}

void UNexusLinkSessionManager::OnSessionCreationComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	}

	OnCreateSessionCompleteDelegateHandle.Reset();

	if (!bWasSuccessful)
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Failed to create session '%s'."), *SessionName.ToString());

		SetSessionState(SessionName, ENexusLinkSessionState::NoSession);
		PendingStartingLevel.Empty();

		SendCreateSessionResult(ENexusLinkCreateSessionResult::Failure);
		return;
	}

	NEXUS_LOG(LogNexusLink, Log, TEXT("Session '%s' created successfully."), *SessionName.ToString());
	SetSessionState(SessionName, ENexusLinkSessionState::Pending);

	// Broadcast before travel so listeners can react (show loading screen, etc).
	SendCreateSessionResult(ENexusLinkCreateSessionResult::Success);

	// Auto-travel to starting level if one was specified.
	if (!PendingStartingLevel.IsEmpty())
	{
		TravelToStartingLevel();
	}
}

void UNexusLinkSessionManager::TravelToStartingLevel()
{
	if (!GameInstanceRef.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("GameInstance is invalid."));
		return;
	}

	UWorld* World = GameInstanceRef->GetWorld();
	if (!World)
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("World is invalid."));
		return;
	}

	// Append ?listen if not already present — the host must be a listen server.
	FString TravelURL = PendingStartingLevel;
	if (!TravelURL.Contains(TEXT("listen")))
	{
		TravelURL.Append(TEXT("?listen"));
	}

	NEXUS_LOG(LogNexusLink, Log, TEXT("Traveling to '%s'..."), *TravelURL);
	GEngine->SetClientTravel(World, *TravelURL, TRAVEL_Absolute);
	PendingStartingLevel.Empty();
}

void UNexusLinkSessionManager::OnSessionFoundComplete(bool bWasSuccessful)
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
		NEXUS_LOG(LogNexusLink, Log, TEXT("Found %d session(s)."), Results.Num());

		CachedSearchResults.Reserve(Results.Num());
		for (const FOnlineSessionSearchResult& Result : Results)
		{
			CachedSearchResults.Emplace(FNexusLinkSearchResult(Result));
		}

		const ENexusLinkFindSessionsResult FinalResult = CachedSearchResults.Num() > 0
			? ENexusLinkFindSessionsResult::Success
			: ENexusLinkFindSessionsResult::NoResults;

		SendFindSessionResult(FinalResult, CachedSearchResults);
	}
	else
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Search failed."));
		SendFindSessionResult(ENexusLinkFindSessionsResult::Failure, CachedSearchResults);
	}
}

void UNexusLinkSessionManager::OnSessionJoinedComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
	}

	ENexusLinkJoinSessionResult FinalResult;

	switch (Result)
	{
	case EOnJoinSessionCompleteResult::Success:
		NEXUS_LOG(LogNexusLink, Log, TEXT("Successfully joined session '%s'."), *SessionName.ToString());
		SetSessionState(SessionName, ENexusLinkSessionState::Pending);
		FinalResult = ENexusLinkJoinSessionResult::Success;
		break;

	case EOnJoinSessionCompleteResult::SessionIsFull:
		NEXUS_LOG(LogNexusLink, Warning, TEXT("Session '%s' is full."), *SessionName.ToString());
		FinalResult = ENexusLinkJoinSessionResult::SessionFull;
		break;

	case EOnJoinSessionCompleteResult::SessionDoesNotExist:
		NEXUS_LOG(LogNexusLink, Warning, TEXT("Session '%s' does not exist."), *SessionName.ToString());
		FinalResult = ENexusLinkJoinSessionResult::SessionNotFound;
		break;

	case EOnJoinSessionCompleteResult::AlreadyInSession:
		NEXUS_LOG(LogNexusLink, Warning, TEXT("Already in session '%s'."), *SessionName.ToString());
		FinalResult = ENexusLinkJoinSessionResult::AlreadyInSession;
		break;

	default:
		NEXUS_LOG(LogNexusLink, Error, TEXT("Failed to join session '%s' (Result: %d)."), *SessionName.ToString(), static_cast<int32>(Result));
		FinalResult = ENexusLinkJoinSessionResult::Failure;
		break;
	}

	// Broadcast before travel so listeners can react (show loading screen, etc).
	SendJoinSessionResult(FinalResult);

	// Auto-travel to the host if join was successful and auto-travel was requested.
	if (FinalResult == ENexusLinkJoinSessionResult::Success && bPendingAutoTravel)
	{
		TravelToSession(SessionName);
	}

	bPendingAutoTravel = false;
}

void UNexusLinkSessionManager::TravelToSession(const FName SessionName)
{
	FString ConnectString;
	if (!GetSessionConnectString(SessionName, ConnectString) || ConnectString.IsEmpty())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Failed to get connect string for session '%s'."), *SessionName.ToString());
		return;
	}

	if (!GameInstanceRef.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("GameInstance is invalid."));
		return;
	}

	UWorld* World = GameInstanceRef->GetWorld();
	if (!World)
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("World is invalid."));
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No PlayerController found."));
		return;
	}

	NEXUS_LOG(LogNexusLink, Log, TEXT("Traveling to '%s'..."), *ConnectString);
	PC->ClientTravel(ConnectString, TRAVEL_Absolute);
}

void UNexusLinkSessionManager::OnSessionDestroyedComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
	}

	SetSessionState(SessionName, ENexusLinkSessionState::NoSession);

	if (bWasSuccessful)
	{
		NEXUS_LOG(LogNexusLink, Log, TEXT("Session '%s' destroyed successfully."), *SessionName.ToString());
		SendDestroySessionResult(ENexusLinkDestroySessionResult::Success);
	}
	else
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Failed to destroy session '%s'."), *SessionName.ToString());
		SendDestroySessionResult(ENexusLinkDestroySessionResult::Failure);
	}
}

void UNexusLinkSessionManager::OnSessionUpdatedComplete(FName SessionName, bool bWasSuccessfully)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(OnSessionUpdateCompleteDelegateHandle);
	}

	OnSessionUpdateCompleteDelegateHandle.Reset();

	if (bWasSuccessfully)
	{
		NEXUS_LOG(LogNexusLink, Log, TEXT("Session '%s' successfully updated."), *SessionName.ToString());
		SendUpdateSessionResult(ENexusLinkUpdateSessionResult::Success);
	}
	else
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Failed to update session '%s'."), *SessionName.ToString());
		SendUpdateSessionResult(ENexusLinkUpdateSessionResult::Failure);
	}
}

void UNexusLinkSessionManager::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	NEXUS_LOG(LogNexusLink, Error, TEXT("Network Error detected: %s"), *ErrorString);

	SetSessionState(ActiveSessionName, ENexusLinkSessionState::NoSession);

	PendingStartingLevel.Empty();
	bPendingAutoTravel = false;

	OnNetworkErrorEvent.Broadcast(ErrorString);
}

void UNexusLinkSessionManager::OnSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType)
{
	NEXUS_LOG(LogNexusLink, Error, TEXT("Session Error detected: %d"), static_cast<int32>(FailureType));

	SetSessionState(ActiveSessionName, ENexusLinkSessionState::NoSession);

	PendingStartingLevel.Empty();
	bPendingAutoTravel = false;

	FString ErrorMessage = TEXT("The online session was lost or the host disconnected.");
	OnNetworkErrorEvent.Broadcast(ErrorMessage);
}
