// Copyright Spike Plugins 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "NexusPartyManager.generated.h"

class UGameInstance;
class UNexusBeaconManager;
class UNexusSessionManager;
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

	/** Fired when a party invite is received from a friend via platform overlay. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Invite Received"))
	FNexusOnPartyInviteReceivedSignature OnPartyInviteReceivedEvent;

	/**
	 * Fired when the party leader creates a game session.
	 * Party members should bind here and call Join Nexus Session with the result,
	 * or override UNexusOnlineContext::OnPartyGameSessionReady for automatic handling.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Party|Events", meta=(DisplayName = "On Party Game Session Ready"))
	FNexusOnPartyGameSessionReadySignature OnPartyGameSessionReadyEvent;
	
public:
	UNexusPartyManager(const FObjectInitializer& ObjectInitializer);
	
	/** Initialize the manager. Called by UNexusOnlineSubsystem. */
	virtual void Initialize(UGameInstance* InGameInstance, UNexusBeaconManager* InBeaconManager, UNexusSessionManager* InSessionManager);
	
	/** Shutdown and release all resources. Called by UNexusOnlineSubsystem. */
	virtual void Deinitialize();
	
	/**
	 * Create a party as the local player (becomes the leader).
	 *
	 * If Params.bCreateLobbySession is true (default), a hidden lobby session is created
	 * alongside the beacon so friends can receive and accept invites via the platform overlay.
	 * This lobby session is separate from your game session.
	 *
	 * @param Params  Party configuration. Use Make Default Party Params for sensible defaults.
	 * @return True if party creation was initiated (lobby session creation is asynchronous).
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party|Leader")
	bool CreateParty(FNexusPartyHostParams Params);

	/** Legacy overload — preserves compatibility. Prefer CreateParty(FNexusPartyHostParams). */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party|Leader", meta = (DeprecatedFunction,
			DeprecationMessage = "Use CreateParty(FNexusPartyHostParams) instead."))
	bool CreatePartyWithSize(int32 MaxSize = 4);
	
	/**
	 * Send a party invite to a friend via the platform overlay (Steam / EOS).
	 * Requires the party to be active (IsInParty() && IsPartyLeader()).
	 *
	 * @param FriendId  Unique ID of the friend to invite.
	 * @return True if the invite was dispatched.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party|Leader")
	bool SendPartyInvite(const FUniqueNetIdRepl& FriendId);

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
	 * Prefer JoinPartyFromSession when you have an FNexusSearchResult — it derives
	 * the beacon address automatically.
	 *
	 * @param HostAddress   IP:port of the party leader's beacon host.
	 * @param LocalPlayerId Local player's unique net ID.
	 * @param DisplayName   Display name for the party roster.
	 * @return True if the connection attempt was initiated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party|Member")
	bool JoinParty(const FString& HostAddress, const FUniqueNetIdRepl& LocalPlayerId, const FString& DisplayName);

	/**
	 * Join a party from an OSS search result (e.g., after accepting a party lobby invite).
	 * The beacon address is derived automatically from the session connection string.
	 *
	 * @param PartyLobbySession  The party's lobby session search result.
	 * @return True if the connection attempt was initiated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party|Member")
	bool JoinPartyFromSession(const FNexusSearchResult& PartyLobbySession);

	/**
	 * Called by UNexusOnlineSubsystem when the local player (as party leader) successfully
	 * creates a game session. Broadcasts OnPartyGameSessionReadyEvent to all party members.
	 * Do not call this manually.
	 */
	void NotifyPartyOfGameSession(const FNexusSearchResult& GameSession);
	
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

	/** Called when the hidden lobby session for the party is created successfully. */
	virtual void OnPartyLobbySessionCreated(FName SessionName, bool bWasSuccessful);

	/** Handles party invite received via platform overlay → fires OnPartyInviteReceivedEvent. */
	UFUNCTION()
	virtual void OnPlatformPartyInviteReceived(const FNexusPendingInvite& Invite);
	
private:
	bool GetLocalPlayerInfo(FUniqueNetIdRepl& OutId, FString& OutDisplayName) const;

	/** Derives the beacon address from a lobby session connect string. */
	bool GetBeaconAddressFromSession(const FNexusSearchResult& Session, FString& OutAddress) const;
	
	void BindPartyHostDelegates();
	void UnbindPartyHostDelegates();
	void BindPartyClientDelegates();
	void CleanupPartyClient();

	/** Creates the hidden lobby session that enables platform overlay invites. */
	bool CreatePartyLobbySession(const FNexusPartyHostParams& Params);

	/** Destroys the party lobby session when the party is disbanded or the leader leaves. */
	void DestroyPartyLobbySession();
	
private:
	TWeakObjectPtr<UGameInstance> GameInstance;
	TWeakObjectPtr<UNexusBeaconManager> BeaconManager;
	TWeakObjectPtr<UNexusSessionManager> SessionManager;
	
	UPROPERTY()
	TObjectPtr<ANexusPartyBeaconHost> PartyHost;
	
	UPROPERTY()
	TObjectPtr<ANexusPartyBeaconClient> PartyClient;
	
	FNexusPartyState CachedPartyState;

	/** Session name used for the hidden party lobby session. */
	static const FName PartyLobbySessionName;

	/** Delegate handle for the party lobby session creation callback. */
	FDelegateHandle LobbySessionCreatedHandle;
	
	FDelegateHandle HostMemberJoinedDelegateHandle;
	FDelegateHandle HostMemberLeftDelegateHandle;
	FDelegateHandle HostStateChangedDelegateHandle;
	
	FDelegateHandle ClientJoinResultDelegateHandle;
	FDelegateHandle ClientStateUpdatedDelegateHandle;
	FDelegateHandle ClientKickedDelegateHandle;
};
