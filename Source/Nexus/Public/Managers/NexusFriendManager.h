// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NexusOnlineTypes.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "NexusFriendManager.generated.h"

class UGameInstance;

/**
 * @class UNexusFriendManager
 * 
 * Manages the friends list, friend presence, and session invites.
 * Supports crossplay by operating through the active Online Subsystem
 * works with Steam, EOS, or any other OSS that implements the friends/session interfaces.
 *
 * Owned by UNexusOnlineSubsystem. Do not instantiate directly.
 */
UCLASS(BlueprintType)
class NEXUS_API UNexusFriendManager : public UObject
{
	GENERATED_BODY()

public:
	/** Fired when the friends list finishes loading. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Friends|Events")
	FNexusOnFriendsListReadySignature OnFriendsListReady;

	/** Fired when a session invite is received from another player. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Friends|Events")
	FNexusOnSessionInviteReceivedSignature OnSessionInviteReceived;

	/** Fired when the local user accepts a session invite (e.g. from Steam Overlay). */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Friends|Events")
	FNexusOnSessionInviteAcceptedSignature OnSessionInviteAccepted;

public:
	UNexusFriendManager();

	/** Initialize the manager. Called by the owning subsystem. */
	void Initialize(UGameInstance* InGameInstance);

	/** Shutdown and clean up all delegates. */
	void Deinitialize();

	/**
	 * Read the friends list from the online subsystem.
	 * This is an async operation results arrive via OnFriendsListReady.
	 *
	 * @return True if the async request was started.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Friends")
	bool ReadFriendsList();

	/** @return The cached friends list. Only valid after a successful ReadFriendsList. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Friends")
	const TArray<FNexusOnlineFriend>& GetFriendsList() const { return CachedFriends; }

	/** @return Number of cached friends. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Friends")
	int32 GetFriendCount() const { return CachedFriends.Num(); }

	/**
	 * Check if a player is on the local user's friend list.
	 *
	 * @param PlayerId The unique net ID to check.
	 * @return True if the player is a friend.
	 */
	UFUNCTION(BlueprintPure, Category = "Nexus|Friends")
	bool IsFriend(const FUniqueNetIdRepl& PlayerId) const;

	/**
	 * Send a session invite to a specific friend.
	 * The friend will receive a platform notification (e.g. Steam toast).
	 *
	 * @param SessionName The session to invite to (usually NAME_GameSession).
	 * @param FriendId Unique net ID of the friend.
	 * @return True if the invite was sent successfully.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Friends")
	bool SendSessionInvite(const FName SessionName, const FUniqueNetIdRepl& FriendId);

	/**
	 * Show the platform-native invite UI (Steam Overlay, EOS Social Panel, etc).
	 * The user picks friends from the platform UI.
	 *
	 * @param SessionName The session to invite for.
	 * @return True if the platform UI was shown.
	 */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Nexus|Friends")
	bool ShowPlatformInviteUI(const FName SessionName);

	/**
	 * Register handlers for incoming session invites and invite acceptance.
	 * Called automatically if bAutoRegisterInviteHandlers is true in settings.
	 */
	void RegisterInviteHandlers();

	/** Unregister all invite handlers. */
	void UnregisterInviteHandlers();

	/** @return The list of pending (not yet accepted) invites. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Friends")
	const TArray<FNexusPendingInvite>& GetPendingInvites() const { return PendingInvites; }

	/** Clear all pending invites. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Friends")
	void ClearPendingInvites() { PendingInvites.Empty(); }

protected:
	/** @return The friends interface from the active OSS. */
	IOnlineFriendsPtr GetFriendsInterface() const;

	/** @return The session interface from the active OSS. */
	IOnlineSessionPtr GetSessionInterface() const;

	/** @return The local player's unique net ID. */
	FUniqueNetIdPtr GetLocalPlayerId() const;

	/** Convert a native FOnlineFriend to our Blueprint wrapper. */
	static FNexusOnlineFriend ConvertFriend(const TSharedRef<FOnlineFriend>& InFriend);

private:
	void OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);
	void OnSessionInviteReceivedInternal(const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FString& AppId, const FOnlineSessionSearchResult& InviteResult);
	void OnSessionUserInviteAcceptedInternal(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);

private:
	UPROPERTY()
	TWeakObjectPtr<UGameInstance> GameInstanceRef;

	/** Cached friends list. Populated after ReadFriendsList completes. */
	TArray<FNexusOnlineFriend> CachedFriends;

	/** Pending invites received but not yet accepted. */
	TArray<FNexusPendingInvite> PendingInvites;

	/** Whether invite handlers are currently registered. */
	bool bInviteHandlersRegistered;

	FDelegateHandle OnSessionInviteReceivedDelegateHandle;
	FDelegateHandle OnSessionUserInviteAcceptedDelegateHandle;
};
