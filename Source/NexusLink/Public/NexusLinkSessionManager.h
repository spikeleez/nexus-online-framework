// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NexusLinkTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NexusLinkSessionManager.generated.h"

class UGameInstance;

/**
 * @class UNexusLinkSessionManager
 * 
 * Manages online session lifecycle: creation, discovery, joining, and destruction.
 * All operations are asynchronous — results are delivered via delegates.
 *
 * Owned by UNexusLinkSubsystem. Do not instantiate directly.
 *
 * Blueprint usage: Get NexusLink Subsystem -> Get Session Manager -> Create/Find/Join/Destroy
 * C++ usage: Bind to NativeOnSessionCreated / NativeOnSessionsFound / etc. for proxy binding.
 */
UCLASS(BlueprintType)
class NEXUSLINK_API UNexusLinkSessionManager : public UObject
{
	GENERATED_BODY()

public:
	/** Fired when a session is created. */
	UPROPERTY(BlueprintAssignable, Category = "NexusLink|Sessions|Events")
	FNexusLinkOnSessionCreated OnSessionCreated;

	/** Fired when sessions are found. */
	UPROPERTY(BlueprintAssignable, Category = "NexusLink|Sessions|Events")
	FNexusLinkOnSessionsFound OnSessionsFound;

	/** Fired when a session is joined. */
	UPROPERTY(BlueprintAssignable, Category = "NexusLink|Sessions|Events")
	FNexusLinkOnSessionJoined OnSessionJoined;

	/** Fired when a session is destroyed. */
	UPROPERTY(BlueprintAssignable, Category = "NexusLink|Sessions|Events")
	FNexusLinkOnSessionDestroyed OnSessionDestroyed;

	/** Fired when the session state changes. */
	UPROPERTY(BlueprintAssignable, Category = "NexusLink|Sessions|Events")
	FNexusLinkOnSessionStateChanged OnSessionStateChanged;

	FNexusLinkNativeOnSessionCreated NativeOnSessionCreated;
	FNexusLinkNativeOnSessionsFound NativeOnSessionsFound;
	FNexusLinkNativeOnSessionJoined NativeOnSessionJoined;
	FNexusLinkNativeOnSessionDestroyed NativeOnSessionDestroyed;

public:
	UNexusLinkSessionManager();

	/** Initialize the manager. Called by the owning subsystem. */
	void Initialize(UGameInstance* InGameInstance);

	/** Shutdown and clean up all delegates. */
	void Deinitialize();

	/**
	 * Create a new online session.
	 * On completion, broadcasts OnSessionCreated and NativeOnSessionCreated.
	 *
	 * @param SessionName Name for the session (usually NAME_GameSession).
	 * @param HostParams Parameters defining the session properties.
	 * @return True if the async request was successfully started.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session")
	bool CreateSession(const FName SessionName, const FNexusLinkHostParams& HostParams);

	/**
	 * Find online sessions matching the given parameters.
	 * On completion, broadcasts OnSessionsFound and NativeOnSessionsFound.
	 *
	 * @param SearchParams Parameters for the search query.
	 * @return True if the async request was successfully started.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session")
	bool FindSessions(const FNexusLinkSearchParams& SearchParams);

	/**
	 * Join a session from a search result.
	 * On completion, broadcasts OnSessionJoined and NativeOnSessionJoined.
	 *
	 * @param SessionName Name for the local session entry (usually NAME_GameSession).
	 * @param SearchResult The search result containing the session to join.
	 * @return True if the async request was successfully started.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session")
	bool JoinSession(const FName SessionName, const FNexusLinkSearchResult& SearchResult, const bool bAutoTravel = true);

	/**
	 * Destroy an active session.
	 * On completion, broadcasts OnSessionDestroyed and NativeOnSessionDestroyed.
	 *
	 * @param SessionName Name of the session to destroy.
	 * @return True if the async request was successfully started.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session")
	bool DestroySession(const FName SessionName);

	/** @return The current session lifecycle state. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session")
	ENexusLinkSessionState GetSessionState() const { return CurrentSessionState; }

	/** @return Whether the local player is currently in an active session. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session")
	bool IsInSession() const;

	/** @return The cached search results from the last FindSessions call. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session")
	const TArray<FNexusLinkSearchResult>& GetLastSearchResults() const { return CachedSearchResults; }

	/**
	 * Get the named session wrapper for an active session.
	 *
	 * @param SessionName The session name to look up.
	 * @return The named session data. Check IsValid() before using.
	 */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session")
	FNexusLinkNamedSession GetNamedSession(const FName SessionName) const;

	/**
	 * Get the platform connect string for a session (used for ClientTravel).
	 *
	 * @param SessionName Name of the session.
	 * @param OutConnectString The resulting connection URL.
	 * @return True if the connect string was retrieved successfully.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session")
	bool GetSessionConnectString(const FName SessionName, FString& OutConnectString) const;

protected:
	/** Update and broadcast the session state. */
	void SetSessionState(const FName SessionName, const ENexusLinkSessionState NewState);

	/** @return The session interface from the active online subsystem. Can be null. */
	IOnlineSessionPtr GetSessionInterface() const;

	/** @return The local player's unique net ID. Can be null. */
	FUniqueNetIdPtr GetLocalPlayerId() const;

	/**
	 * Broadcast a result to both the dynamic (Blueprint) and native (C++) delegates.
	 * Centralizes the dual-broadcast pattern used by all operations.
	 */
	void BroadcastCreateResult(const ENexusLinkCreateSessionResult Result);
	void BroadcastFindResult(const ENexusLinkFindSessionsResult Result, const TArray<FNexusLinkSearchResult>& Results);
	void BroadcastJoinResult(const ENexusLinkJoinSessionResult Result);
	void BroadcastDestroyResult(const ENexusLinkDestroySessionResult Result);

private:
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	
	/** Perform ClientTravel to the host after a successful join. */
	void TravelToSession(const FName SessionName);

	/** Perform ServerTravel to the starting level after session creation. */
	void TravelToStartingLevel();

private:
	/** Weak reference to the owning game instance. */
	UPROPERTY()
	TWeakObjectPtr<UGameInstance> GameInstanceRef;

	/** Current session lifecycle state. */
	ENexusLinkSessionState CurrentSessionState;

	/** Shared pointer to the active search settings for the current FindSessions operation. */
	TSharedPtr<FOnlineSessionSearch> CurrentSearchSettings;

	/** Cached search results from the last successful FindSessions. */
	TArray<FNexusLinkSearchResult> CachedSearchResults;

	/** Session name for the currently active async operation. */
	FName ActiveSessionName;

	/** Whether the current join operation should auto-travel on success. */
	uint8 bPendingAutoTravel:1;

	/** Cached starting level from the current create operation. */
	FString PendingStartingLevel;

	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;
};
