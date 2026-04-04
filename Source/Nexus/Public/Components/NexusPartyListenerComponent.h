// Copyright Spike Plugins 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "Components/ActorComponent.h"
#include "NexusPartyListenerComponent.generated.h"

/**
 * @class UNexusPartyListenerComponent
 *
 * Drop-in ActorComponent that forwards all UNexusPartyManager events to
 * Blueprint-assignable delegates. Add this to any Actor that needs to react
 * to party lifecycle events (roster changes, kicks, disbands, invites, etc.).
 *
 * Binds on BeginPlay, unbinds on EndPlay.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(NexusOnline), meta=(BlueprintSpawnableComponent))
class NEXUS_API UNexusPartyListenerComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	/** Forwarded from UNexusPartyManager::OnPartyCreatedEvent. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Created"))
	FNexusOnPartyCreatedSignature OnPartyCreatedEvent;

	/** Forwarded from UNexusPartyManager::OnPartyJoinedEvent. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Joined"))
	FNexusOnPartyJoinedSignature OnPartyJoinedEvent;

	/**
	 * Forwarded from UNexusPartyManager::OnPartyStateUpdatedEvent.
	 * Fires whenever the roster changes for any reason.
	 * Use this to refresh the party UI panel.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party State Updated"))
	FNexusOnPartyStateUpdatedSignature OnPartyStateUpdatedEvent;

	/** Forwarded from UNexusPartyManager::OnPartyMemberJoinedEvent. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Member Joined"))
	FNexusOnPartyMemberJoinedSignature OnPartyMemberJoinedEvent;

	/** Forwarded from UNexusPartyManager::OnPartyMemberLeftEvent. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Member Left"))
	FNexusOnPartyMemberLeftSignature OnPartyMemberLeftEvent;

	/** Forwarded from UNexusPartyManager::OnPartyDisbandedEvent. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Disbanded"))
	FNexusOnPartyDisbandedSignature OnPartyDisbandedEvent;

	/** Forwarded from UNexusPartyManager::OnPartyInviteReceivedEvent. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Invite Received"))
	FNexusOnPartyInviteReceivedSignature OnPartyInviteReceivedEvent;

	/**
	 * Forwarded from UNexusPartyManager::OnPartyGameSessionReadyEvent.
	 * Fires on party members when the leader creates a game session.
	 * Bind here and call Join Nexus Session with the provided result to follow the leader.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Game Session Ready"))
	FNexusOnPartyGameSessionReadySignature OnPartyGameSessionReadyEvent;

public:
	UNexusPartyListenerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface
	
protected:
	UFUNCTION()
	virtual void OnPartyCreated(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState);
	
	UFUNCTION()
	virtual void OnPartyJoined(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState);
	
	UFUNCTION()
	virtual void OnPartyStateUpdated(const FNexusPartyState& PartyState);
	
	UFUNCTION()
	virtual void OnPartyMemberJoined(const FNexusPartySlot& NewMember, const FNexusPartyState& UpdatedState);
	
	UFUNCTION()
	virtual void OnPartyMemberLeft(const FUniqueNetIdRepl& MemberId, ENexusPartyMemberStatus MemberStatus);
	
	UFUNCTION()
	virtual void OnPartyDisbanded(ENexusPartyResult PartyResult);

	UFUNCTION()
	virtual void OnPartyInviteReceived(const FNexusPendingPartyInvite& Invite);

	UFUNCTION()
	virtual void OnPartyGameSessionReady(const FNexusSearchResult& GameSession);
	
private:
	void BindToManagers();
	void UnbindFromManagers();
	
private:
	/** Tracks whether we are currently bound to avoid double-bind or double-unbind. */
	uint8 bIsBound:1;
};
