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

	NEXUS_LOG(LogNexusLink, Log, TEXT("Initialized."));
}

void UNexusLinkSessionManager::Deinitialize()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
	}

	CurrentSearchSettings.Reset();
	CachedSearchResults.Empty();

	NEXUS_LOG(LogNexusLink, Log, TEXT("Deinitialized."));
}

bool UNexusLinkSessionManager::CreateSession(const FName SessionName, const FNexusLinkHostParams& HostParams)
{
	if (!HostParams.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Invalid host params."));
		BroadcastCreateResult(ENexusLinkCreateSessionResult::Failure);
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No valid session interface."));
		BroadcastCreateResult(ENexusLinkCreateSessionResult::NoOnlineSubsystem);
		return false;
	}

	// Check if a session with this name already exists.
	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession != nullptr)
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("Session '%s' already exists. Destroy it first."), *SessionName.ToString());
		BroadcastCreateResult(ENexusLinkCreateSessionResult::AlreadyExists);
		return false;
	}

	// Get local player ID.
	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Could not get local player unique ID."));
		BroadcastCreateResult(ENexusLinkCreateSessionResult::Failure);
		return false;
	}

	// Build session settings from host params.
	FOnlineSessionSettings NewSettings;
	HostParams.ToOnlineSessionSettings(NewSettings);

	ActiveSessionName = SessionName;
	PendingStartingLevel = HostParams.StartingLevel;

	// Bind completion delegate.
	OnCreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)
	);

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
		BroadcastCreateResult(ENexusLinkCreateSessionResult::Failure);
		return false;
	}

	return true;
}

bool UNexusLinkSessionManager::FindSessions(const FNexusLinkSearchParams& SearchParams)
{
	if (!SearchParams.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Invalid search params."));
		BroadcastFindResult(ENexusLinkFindSessionsResult::Failure, TArray<FNexusLinkSearchResult>());
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No valid session interface."));
		BroadcastFindResult(ENexusLinkFindSessionsResult::NoOnlineSubsystem, TArray<FNexusLinkSearchResult>());
		return false;
	}

	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Could not get local player unique ID."));
		BroadcastFindResult(ENexusLinkFindSessionsResult::Failure, TArray<FNexusLinkSearchResult>());
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
	OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)
	);

	NEXUS_LOG(LogNexusLink, Log, TEXT("Searching (MaxResults: %d, LAN: %s, Presence: %s)..."),
		SearchParams.MaxSearchResults,
		SearchParams.bIsLanQuery ? TEXT("true") : TEXT("false"),
		SearchParams.bSearchPresence ? TEXT("true") : TEXT("false"));

	if (!SessionInterface->FindSessions(*LocalPlayerId, CurrentSearchSettings.ToSharedRef()))
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Platform FindSessions call failed."));
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		BroadcastFindResult(ENexusLinkFindSessionsResult::Failure, TArray<FNexusLinkSearchResult>());
		return false;
	}

	return true;
}

bool UNexusLinkSessionManager::JoinSession(const FName SessionName, const FNexusLinkSearchResult& SearchResult, const bool bAutoTravel /*= true*/)
{
	if (!SearchResult.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Invalid search result."));
		BroadcastJoinResult(ENexusLinkJoinSessionResult::Failure);
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No valid session interface."));
		BroadcastJoinResult(ENexusLinkJoinSessionResult::NoOnlineSubsystem);
		return false;
	}

	// Prevent joining if already in a session with this name.
	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession != nullptr)
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("Already in session '%s'. Destroy it first."), *SessionName.ToString());
		BroadcastJoinResult(ENexusLinkJoinSessionResult::AlreadyInSession);
		return false;
	}

	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Could not get local player unique ID."));
		BroadcastJoinResult(ENexusLinkJoinSessionResult::Failure);
		return false;
	}

	ActiveSessionName = SessionName;
	bPendingAutoTravel = bAutoTravel;

	// Bind completion delegate.
	OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)
	);

	NEXUS_LOG(LogNexusLink, Log, TEXT("Joining session '%s' (Host: %s, AutoTravel: %s)..."), *SessionName.ToString(), *SearchResult.GetOwnerUsername(), bAutoTravel ? TEXT("true") : TEXT("false"));

	if (!SessionInterface->JoinSession(*LocalPlayerId, SessionName, SearchResult.OnlineResult))
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Platform JoinSession call failed."));
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		bPendingAutoTravel = false;
		BroadcastJoinResult(ENexusLinkJoinSessionResult::Failure);
		return false;
	}

	return true;
}

bool UNexusLinkSessionManager::DestroySession(const FName SessionName)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("DestroySession: No valid session interface."));
		BroadcastDestroyResult(ENexusLinkDestroySessionResult::Failure);
		return false;
	}

	const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionName);
	if (ExistingSession == nullptr)
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("DestroySession: No session '%s' to destroy."), *SessionName.ToString());
		BroadcastDestroyResult(ENexusLinkDestroySessionResult::NoSession);
		return false;
	}

	ActiveSessionName = SessionName;
	SetSessionState(SessionName, ENexusLinkSessionState::Destroying);

	// Bind completion delegate.
	OnDestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)
	);

	NEXUS_LOG(LogNexusLink, Log, TEXT("DestroySession: Destroying session '%s'..."), *SessionName.ToString());

	if (!SessionInterface->DestroySession(SessionName))
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("DestroySession: Platform DestroySession call failed."));
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		SetSessionState(SessionName, ENexusLinkSessionState::NoSession);
		BroadcastDestroyResult(ENexusLinkDestroySessionResult::Failure);
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
		OnSessionStateChanged.Broadcast(SessionName, NewState);
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

void UNexusLinkSessionManager::BroadcastCreateResult(const ENexusLinkCreateSessionResult Result)
{
	OnSessionCreated.Broadcast(Result);
	NativeOnSessionCreated.Broadcast(Result);
}

void UNexusLinkSessionManager::BroadcastFindResult(const ENexusLinkFindSessionsResult Result, const TArray<FNexusLinkSearchResult>& Results)
{
	OnSessionsFound.Broadcast(Result, Results);
	NativeOnSessionsFound.Broadcast(Result, Results);
}

void UNexusLinkSessionManager::BroadcastJoinResult(const ENexusLinkJoinSessionResult Result)
{
	OnSessionJoined.Broadcast(Result);
	NativeOnSessionJoined.Broadcast(Result);
}

void UNexusLinkSessionManager::BroadcastDestroyResult(const ENexusLinkDestroySessionResult Result)
{
	OnSessionDestroyed.Broadcast(Result);
	NativeOnSessionDestroyed.Broadcast(Result);
}

void UNexusLinkSessionManager::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	}

	if (bWasSuccessful)
	{
		NEXUS_LOG(LogNexusLink, Log, TEXT("Session '%s' created successfully."), *SessionName.ToString());
		SetSessionState(SessionName, ENexusLinkSessionState::Pending);

		// Broadcast before travel so listeners can react (show loading screen, etc).
		BroadcastCreateResult(ENexusLinkCreateSessionResult::Success);

		// Auto-travel to starting level if one was specified.
		if (!PendingStartingLevel.IsEmpty())
		{
			TravelToStartingLevel();
		}
	}
	else
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Failed to create session '%s'."), *SessionName.ToString());
		SetSessionState(SessionName, ENexusLinkSessionState::NoSession);
		BroadcastCreateResult(ENexusLinkCreateSessionResult::Failure);
	}

	PendingStartingLevel.Empty();
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
}

void UNexusLinkSessionManager::OnFindSessionsComplete(bool bWasSuccessful)
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

		BroadcastFindResult(FinalResult, CachedSearchResults);
	}
	else
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Search failed."));
		BroadcastFindResult(ENexusLinkFindSessionsResult::Failure, CachedSearchResults);
	}
}

void UNexusLinkSessionManager::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
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
	BroadcastJoinResult(FinalResult);

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

void UNexusLinkSessionManager::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
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
		BroadcastDestroyResult(ENexusLinkDestroySessionResult::Success);
	}
	else
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Failed to destroy session '%s'."), *SessionName.ToString());
		BroadcastDestroyResult(ENexusLinkDestroySessionResult::Failure);
	}
}
