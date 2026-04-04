// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "NexusOnlineContext.generated.h"

class UGameInstance;

/**
 * @class UNexusOnlineContext
 *
 * The "online brain" of your game. Create a Blueprint (or C++) subclass, register it in:
 *   Project Settings -> Nexus Online Settings -> Context -> Online Context Class
 *
 * Nexus automatically instantiates this class when the GameInstance initializes and
 * wires it to all important online events. Override only the functions you need —
 * each has an empty default implementation, so no Super:: call is required.
 *
 * Blueprint access: UNexusOnlineLibrary::GetNexusContext(WorldContextObject)
 */
UCLASS(BlueprintType, Blueprintable, Abstract)
class NEXUS_API UNexusOnlineContext : public UObject
{
	GENERATED_BODY()

public:
	//~Begin UObject interface
	virtual UWorld* GetWorld() const override;
	//~End UObject interface

	/** Called once all Nexus managers have been initialized. */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context", meta=(DisplayName = "On Nexus Online Context Initialized"))
	void OnNexusInitialized();
	virtual void OnNexusInitialized_Implementation() {}

public:
	/**
	 * Called when a session invite is received from a friend via the platform overlay.
	 * Override to show a UI notification.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context|Session", meta=(DisplayName = "On Session Invite Received"))
	void OnSessionInviteReceived(const FNexusPendingInvite& Invite);
	virtual void OnSessionInviteReceived_Implementation(const FNexusPendingInvite& Invite) {}

	/**
	 * Called when the local player accepts a session invite (e.g., Steam Overlay).
	 * Override if you want manual control before joining.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context|Session", meta=(DisplayName = "On Session Invite Accepted"))
	void OnSessionInviteAccepted(const FNexusSearchResult& InviteSession);
	virtual void OnSessionInviteAccepted_Implementation(const FNexusSearchResult& InviteSession) {}

	/**
	 * Called when the network connection fails or the session is terminated unexpectedly.
	 * Override to return the player to the main menu.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context|Session", meta=(DisplayName = "On Connection Network Error"))
	void OnNetworkError(const FString& ErrorMessage);
	virtual void OnNetworkError_Implementation(const FString& ErrorMessage) {}

	/**
	 * Called when the active game session is destroyed (host quit, timeout, etc.).
	 * Override to show a reconnect prompt or return to main menu.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context|Session", meta=(DisplayName = "On Session Lost"))
	void OnSessionLost();
	virtual void OnSessionLost_Implementation() {}

public:
	/**
	 * Called when a party invite is received from a friend.
	 * Override to display a notification widget and give the player Accept/Decline options.
	 * Pass the Invite directly to Accept Nexus Party Invite — no extra setup needed.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context|Party", meta=(DisplayName = "On Party Invite Received"))
	void OnPartyInviteReceived(const FNexusPendingPartyInvite& Invite);
	virtual void OnPartyInviteReceived_Implementation(const FNexusPendingPartyInvite& Invite) {}

	/**
	 * Called when the local player is kicked from the party by the leader.
	 * Override to show a message and return to the main menu.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context|Party", meta=(DisplayName = "On Kicked From Party"))
	void OnKickedFromParty();
	virtual void OnKickedFromParty_Implementation() {}

	/**
	 * Called when the party is disbanded (leader left or called DisbandParty()).
	 * Override to clean up party-related UI.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context|Party", meta=(DisplayName = "On Party Disbanded"))
	void OnPartyDisbanded();
	virtual void OnPartyDisbanded_Implementation() {}

	/**
	 * Called on all party members when a new member joins.
	 * Override to update the party roster UI.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context|Party", meta=(DisplayName = "On Party Member Joined"))
	void OnPartyMemberJoined(const FNexusPartySlot& NewMember);
	virtual void OnPartyMemberJoined_Implementation(const FNexusPartySlot& NewMember) {}

	/**
	 * Called when a party member leaves or is kicked.
	 * Override to update the party roster UI.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context|Party", meta=(DisplayName = "On Party Member Left"))
	void OnPartyMemberLeft(const FUniqueNetIdRepl& MemberId, ENexusPartyMemberStatus Reason);
	virtual void OnPartyMemberLeft_Implementation(const FUniqueNetIdRepl& MemberId, ENexusPartyMemberStatus Reason) {}

	/**
	 * Called when the party leader creates a game session.
	 * For members: join the session to follow the leader into the game.
	 * Default for members: calls JoinNexusSession automatically.
	 * Override if you want a confirm dialog before traveling.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Context|Party", meta=(DisplayName = "On Party Game Session Ready"))
	void OnPartyGameSessionReady(const FNexusSearchResult& GameSession);
	virtual void OnPartyGameSessionReady_Implementation(const FNexusSearchResult& GameSession) {}

public:
	/** Initialize. Called by UNexusOnlineSubsystem after all managers are ready. */
	virtual void Initialize(UGameInstance* InGameInstance);

	/** Deinitialize. Called by UNexusOnlineSubsystem on shutdown. */
	virtual void Deinitialize();

	/** @return The owning GameInstance. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Context")
	UGameInstance* GetGameInstance() const;

	// Called by the subsystem delegate bindings. They adapt the engine delegate
	// signatures to the BlueprintNativeEvent signatures above.

	UFUNCTION()
	void OnSessionDestroyedForContext(ENexusDestroySessionResult Result);

	UFUNCTION()
	void OnPartyDisbandedForContext(ENexusPartyResult Reason);

	UFUNCTION()
	void OnPartyMemberJoinedForContext(const FNexusPartySlot& NewMember, const FNexusPartyState& UpdatedState);

	UFUNCTION()
	void OnPartyMemberLeftForContext(const FUniqueNetIdRepl& MemberId, ENexusPartyMemberStatus Reason);

private:
	TWeakObjectPtr<UGameInstance> CachedGameInstance;
};
