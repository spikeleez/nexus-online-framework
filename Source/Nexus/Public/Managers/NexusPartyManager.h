// Copyright Spike Plugins 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NexusOnlineTypes.h"
#include "NexusPartyManager.generated.h"

class UGameInstance;
class UNexusBeaconManager;
class ANexusPartyBeaconHost;
class ANexusPartyBeaconClient;

/**
 * @class UNexusPartyManager
 *
 * Manages the lifecycle of a persistent multiplayer party.
 * Owned by UNexusOnlineSubsystem.
 *
 * Role model:
 *   LEADER  — Calls CreateParty(). Owns ANexusPartyBeaconHost on the beacon host.
 *             Can kick members and disband the party.
 *   MEMBER  — Calls JoinParty(). Owns an ANexusPartyBeaconClient connected to the leader.
 *
 * The party persists across game session boundaries (Hub <-> run levels).
 * The leader's beacon host runs independently of the game session.
 *
 * Blueprint access: UNexusOnlineLibrary::GetPartyManager().
 */
UCLASS(BlueprintType, Blueprintable)
class NEXUS_API UNexusPartyManager : public UObject
{
	GENERATED_BODY()
	
public:
	/** Fired when the local player creates (or fails to create) a party. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Created"))
	FNexusOnPartyCreatedSignature OnPartyCreatedEvent;

	/** Fired when the local player joins (or fails to join) a party. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Joined"))
	FNexusOnPartyJoinedSignature OnPartyJoinedEvent;

	/** Fired whenever the party state changes for any reason. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party State Updated"))
	FNexusOnPartyStateUpdatedSignature OnPartyStateUpdatedEvent;

	/** Fired when a new member successfully joins the party. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Member Joined"))
	FNexusOnPartyMemberJoinedSignature OnPartyMemberJoinedEvent;

	/** Fired when a member leaves or is kicked from the party. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Member Left"))
	FNexusOnPartyMemberLeftSignature OnPartyMemberLeftEvent;

	/** Fired when the party is disbanded or the local player is kicked. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Disbanded"))
	FNexusOnPartyDisbandedSignature OnPartyDisbandedEvent;
	
public:
	UNexusPartyManager(const FObjectInitializer& ObjectInitializer);
	
	/** Initialize the manager. Called by UNexusOnlineSubsystem. */
	virtual void Initialize(UGameInstance* InGameInstance, UNexusBeaconManager* InBeaconManager);
	
	/** Shutdown and release all resources. Called by UNexusOnlineSubsystem. */
	virtual void Deinitialize();
	
	/**
	 * Create a party as the local player (becomes the leader).
	 * Ensures the beacon host is running, then activates the party host object.
	 * Result fires synchronously via OnPartyCreated.
	 *
	 * @param MaxSize  Maximum party size including the leader. Clamped to [2, 8].
	 * @return True if party creation succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party|Leader")
	bool CreateParty(int32 MaxSize = 4);
	
	/**
	 * Disband the party (leader only).
	 * All members receive a kick notification. OnPartyDisbanded fires.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party|Leader")
	void DisbandParty();
	
	/**
	 * Kick a member from the party (leader only).
	 *
	 * @param MemberId  Unique ID of the member to kick.
	 * @return True if the member was found and kicked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party|Leader")
	bool KickMember(const FUniqueNetIdRepl& MemberId);
	
	/**
	 * Join a party hosted by another player (async).
	 * Result fires via OnPartyJoined after the beacon handshake completes.
	 *
	 * @param HostAddress   IP address of the party leader's beacon host.
	 * @param LocalPlayerId Local player's unique net ID.
	 * @param DisplayName   Display name for the party roster.
	 * @return True if the connection attempt was initiated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party|Member")
	bool JoinParty(const FString& HostAddress, const FUniqueNetIdRepl& LocalPlayerId, const FString& DisplayName);
	
	/**
	 * Voluntarily leave the current party (member only).
	 * The server is notified before the beacon is destroyed. OnPartyDisbanded fires locally.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party|Member")
	void LeaveParty();
	
	/** @return Whether the local player is currently in an active party. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Party|State")
	bool IsInParty() const;
	
	/** @return Whether the local player is the party leader. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Party|State")
	bool IsPartyLeader() const;
	
	/** @return A copy of the current party state. Only valid when IsInParty() is true. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Party|State")
	FORCEINLINE FNexusPartyState GetPartyState() const { return CachedPartyState; }
	
	ANexusPartyBeaconHost* GetPartyHost() const { return PartyHost; }
	ANexusPartyBeaconClient* GetPartyClient() const { return PartyClient; }
	
protected:
	virtual void OnHostMemberJoined(const FUniqueNetIdRepl& MemberId, const FNexusPartyState& PartyState);
	virtual void OnHostMemberLeft(const FUniqueNetIdRepl& MemberId, ENexusPartyMemberStatus MemberState);
	virtual void OnHostStateChanged(const FNexusPartyState& PartyState);
	
	virtual void OnClientJoinResult(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState);
	virtual void OnClientStateUpdated(const FNexusPartyState& PartyState);
	virtual void OnClientKicked();
	
private:
	/** Retrieve the local player's unique ID and display name from the GameInstance. */
	bool GetLocalPlayerInfo(FUniqueNetIdRepl& OutId, FString& OutDisplayName) const;
	
	void BindPartyHostDelegates();
	void UnbindPartyHostDelegates();
	void BindPartyClientDelegates();
	
	/** Unbind client delegates and destroy the client beacon actor. */
	void CleanupPartyClient();
	
private:
	TWeakObjectPtr<UGameInstance> GameInstance;
	TWeakObjectPtr<UNexusBeaconManager> BeaconManager;
	
	/**
	 * Reference to the party host object on the beacon host.
	 * Valid only when the local player is the party leader and a party is active.
	 */
	UPROPERTY()
	TObjectPtr<ANexusPartyBeaconHost> PartyHost;
	
	/**
	 * The client beacon for maintaining presence in another player's party.
	 * Valid only when the local player is a non-leader party member.
	 */
	UPROPERTY()
	TObjectPtr<ANexusPartyBeaconClient> PartyClient;
	
	/** Cached snapshot of the party state. Mirrors the authoritative state. */
	FNexusPartyState CachedPartyState;
	
	// Delegate handles for host object
	FDelegateHandle HostMemberJoinedDelegateHandle;
	FDelegateHandle HostMemberLeftDelegateHandle;
	FDelegateHandle HostStateChangedDelegateHandle;
	
	// Delegate handles for party client
	FDelegateHandle ClientJoinResultDelegateHandle;
	FDelegateHandle ClientStateUpdatedDelegateHandle;
	FDelegateHandle ClientKickedDelegateHandle;
};
