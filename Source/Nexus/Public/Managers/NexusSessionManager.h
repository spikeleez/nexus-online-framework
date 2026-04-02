// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NexusOnlineTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NexusSessionManager.generated.h"

class UGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusOnNetworkErrorSignature, const FString&, ErrorMessage);

/**
 * @class UNexusSessionManager
 * 
 * Manages online session lifecycle: creation, discovery, joining, and destruction.
 * All operations are asynchronous results are delivered via delegates.
 *
 * Owned by UNexusOnlineSubsystem. Do not instantiate directly.
 *
 * Blueprint usage: Get Nexus Subsystem -> Get Session Manager -> Create/Find/Join/Destroy
 * C++ usage: Bind to NativeOnSessionCreated / NativeOnSessionsFound / etc. for proxy binding.
 */
UCLASS(BlueprintType)
class NEXUS_API UNexusSessionManager : public UObject
{
	GENERATED_BODY()

public:
	/** Fired when a session is created. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Sessions|Events", meta = (DisplayName = "On Session Created"))
	FNexusOnSessionCreatedSignature OnSessionCreatedEvent;

	/** Fired when sessions are found. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Sessions|Events", meta = (DisplayName = "On Session Found"))
	FNexusOnSessionsFoundSignature OnSessionsFoundEvent;

	/** Fired when a session is joined. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Sessions|Events", meta = (DisplayName = "On Session Joined"))
	FNexusOnSessionJoinedSignature OnSessionJoinedEvent;

	/** Fired when a session is destroyed. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Sessions|Events", meta = (DisplayName = "On Session Destroyed"))
	FNexusOnSessionDestroyedSignature OnSessionDestroyedEvent;

	/** Fired when the session state changes. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Sessions|Events", meta = (DisplayName = "On Session State Changed"))
	FNexusOnSessionStateChangedSignature OnSessionStateChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Nexus|Sessions|Events", meta = (DisplayName = "On Network Error"))
	FNexusOnNetworkErrorSignature OnNetworkErrorEvent;

	UPROPERTY(BlueprintAssignable, Category = "Nexus|Sessions|Events", meta = (DisplayName = "On Session Updated"))
	FNexusOnSessionUpdatedSignature OnSessionUpdatedEvent;

	FNexusNativeOnSessionCreatedSignature NativeOnSessionCreated;
	FNexusNativeOnSessionsFoundSignature NativeOnSessionsFound;
	FNexusNativeOnSessionJoinedSignature NativeOnSessionJoined;
	FNexusNativeOnSessionDestroyedSignature NativeOnSessionDestroyed;
	FNexusNativeOnSessionUpdatedSignature NativeOnSessionUpdated;

public:
	UNexusSessionManager();

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
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session")
	bool CreateSession(const FName SessionName, const FNexuHostParams& HostParams);

	/**
	 * Updates the settings of an active session (e.g., change map, change max players).
	 *
	 * @param SessionName Name of the session to be updated.
	 * @param NewHostParams New parameters to override the old ones.
	 * @return True if the asynchronous request was successfully initiated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session")
	bool UpdateSession(const FName SessionName, const FNexuHostParams& NewHostParams);

	/**
	 * Find online sessions matching the given parameters.
	 * On completion, broadcasts OnSessionsFound and NativeOnSessionsFound.
	 *
	 * @param SearchParams Parameters for the search query.
	 * @return True if the async request was successfully started.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session")
	bool FindSessions(const FNexusSearchParams& SearchParams);

	/**
	 * Join a session from a search result.
	 * On completion, broadcasts OnSessionJoined and NativeOnSessionJoined.
	 *
	 * @param SessionName Name for the local session entry (usually NAME_GameSession).
	 * @param SearchResult The search result containing the session to join.
	 * @return True if the async request was successfully started.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session")
	bool JoinSession(const FName SessionName, const FNexusSearchResult& SearchResult, const bool bAutoTravel = true);

	/**
	 * Destroy an active session.
	 * On completion, broadcasts OnSessionDestroyed and NativeOnSessionDestroyed.
	 *
	 * @param SessionName Name of the session to destroy.
	 * @return True if the async request was successfully started.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session")
	bool DestroySession(const FName SessionName);

	/** @return The current session lifecycle state. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Session")
	ENexusSessionState GetSessionState() const { return CurrentSessionState; }

	/** @return Whether the local player is currently in an active session. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Session")
	bool IsInSession() const;

	/** @return The cached search results from the last FindSessions call. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Session")
	const TArray<FNexusSearchResult>& GetLastSearchResults() const { return CachedSearchResults; }

	/**
	 * Get the named session wrapper for an active session.
	 *
	 * @param SessionName The session name to look up.
	 * @return The named session data. Check IsValid() before using.
	 */
	UFUNCTION(BlueprintPure, Category = "Nexus|Session")
	FNexusNamedSession GetNamedSession(const FName SessionName) const;

	/**
	 * Get the platform connect string for a session (used for ClientTravel).
	 *
	 * @param SessionName Name of the session.
	 * @param OutConnectString The resulting connection URL.
	 * @return True if the connect string was retrieved successfully.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session")
	bool GetSessionConnectString(const FName SessionName, FString& OutConnectString) const;

protected:
	/** Update and broadcast the session state. */
	void SetSessionState(const FName SessionName, const ENexusSessionState NewState);

	/** @return The session interface from the active online subsystem. Can be null. */
	IOnlineSessionPtr GetSessionInterface() const;

	/** @return The local player's unique net ID. Can be null. */
	FUniqueNetIdPtr GetLocalPlayerId() const;

	/**
	 * Broadcast a result to both the dynamic (Blueprint) and native (C++) delegates.
	 * Centralizes the dual-broadcast pattern used by all operations.
	 */
	void SendCreateSessionResult(const ENexusCreateSessionResult Result);
	void SendFindSessionResult(const ENexusFindSessionsResult Result, const TArray<FNexusSearchResult>& Results);
	void SendJoinSessionResult(const ENexusJoinSessionResult Result);
	void SendDestroySessionResult(const ENexusDestroySessionResult Result);
	void SendUpdateSessionResult(const ENexusUpdateSessionResult Result);

private:
	virtual void OnSessionCreationComplete(FName SessionName, bool bWasSuccessful);
	virtual void OnSessionFoundComplete(bool bWasSuccessful);
	virtual void OnSessionJoinedComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	virtual void OnSessionDestroyedComplete(FName SessionName, bool bWasSuccessful);
	virtual void OnSessionUpdatedComplete(FName SessionName, bool bWasSuccessfully);

	virtual void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	virtual void OnSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType);
	
	/** Perform ClientTravel to the host after a successful join. */
	void TravelToSession(const FName SessionName);

	/** Perform ServerTravel to the starting level after session creation. */
	void TravelToStartingLevel();

private:
	/** Weak reference to the owning game instance. */
	UPROPERTY()
	TWeakObjectPtr<UGameInstance> GameInstanceRef;

	/** Current session lifecycle state. */
	ENexusSessionState CurrentSessionState;

	/** Shared pointer to the active search settings for the current FindSessions operation. */
	TSharedPtr<FOnlineSessionSearch> CurrentSearchSettings;

	/** Cached search results from the last successful FindSessions. */
	TArray<FNexusSearchResult> CachedSearchResults;

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
	FDelegateHandle OnNetworkFailureDelegateHandle;
	FDelegateHandle OnSessionFailureDelegateHandle;
	FDelegateHandle OnSessionUpdateCompleteDelegateHandle;
};
