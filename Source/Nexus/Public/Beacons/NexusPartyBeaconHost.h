// Copyright Spike Plugins 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "NexusOnlineTypes.h"
#include "NexusPartyBeaconHost.generated.h"

class ANexusPartyBeaconClient;

/** Fired when a new member is accepted into the party. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FNexusPartyHostNativeMemberJoinedSignature, const FUniqueNetIdRepl& /*MemberId*/, const FNexusPartyState& /*UpdateState*/);

/** Fired when a member leaves or is kicked from the party. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FNexusPartyHostNativeMemberLeftSignature, const FUniqueNetIdRepl& /*MemberId*/, ENexusPartyMemberStatus /*MemberStatus*/);

/** Fired whenever the authoritative party state changes. */
DECLARE_MULTICAST_DELEGATE_OneParam(FNexusPartyHostNativeStateChangedSignature, const FNexusPartyState& /*PartyState*/);

/**
 * @class ANexusPartyBeaconHost
 *
 * Server-side host object for party management via Online Beacons.
 * Lives on the party leader's machine, registered on AOnlineBeaconHost.
 *
 * Responsibilities:
 *   - Owns the authoritative FNexusPartyState.
 *   - Accepts or rejects join requests from ANexusPartyBeaconClient actors.
 *   - Broadcasts full state snapshots to all connected members on every change.
 *   - Handles graceful leaves and leader-initiated kicks.
 *
 * Lifecycle:
 *   - Spawned + registered by UNexusBeaconManager when the beacon host starts.
 *   - Activated by UNexusPartyManager::CreateParty() via InitializeParty().
 *   - Deactivated by DisbandParty(). The actor remains alive until the beacon host stops.
 */
UCLASS(BlueprintType, Blueprintable)
class NEXUS_API ANexusPartyBeaconHost : public AOnlineBeaconHostObject
{
	GENERATED_BODY()
	
public:
	FNexusPartyHostNativeMemberJoinedSignature NativeOnMemberJoinedEvent;
	FNexusPartyHostNativeMemberLeftSignature NativeOnMemberLeftEvent;
	FNexusPartyHostNativeStateChangedSignature NativeOnStateChangedEvent;

public:
	ANexusPartyBeaconHost(const FObjectInitializer& ObjectInitializer);
	
	//~Begin AOnlineBeaconHostObject interface
	virtual void OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection) override;
	virtual void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;
	//~End of AOnlineBeaconHostObject interface
	
	/**
	 * Activate the party host with the given leader and configuration.
	 * Must be called before any clients can join.
	 *
	 * @param LeaderId    The party leader's unique net ID. Must be valid.
	 * @param LeaderName  Display name of the leader.
	 * @param InMaxSize   Maximum party size including the leader. Clamped to [2, 8].
	 */
	virtual void InitializeParty(const FUniqueNetIdRepl& LeaderId, const FString& LeaderName, int32 InMaxSize);
	
	/**
	 * Disband the active party.
	 * All connected members receive a kick notification and are disconnected.
	 * The host object reverts to an inactive (dormant) state.
	 */
	virtual void DisbandParty();
	
	/**
	 * Kick a party member by their unique ID. Leader-only operation.
	 *
	 * @param MemberId  Unique ID of the member to kick.
	 * @return True if the member was found and kicked.
	 */
	virtual bool KickMember(const FUniqueNetIdRepl& MemberId);
	
	/** @return True if a party has been initialized and is accepting joins. */
	bool IsPartyActive() const { return bPartyActive; }
	
	/** @return Read-only reference to the current authoritative party state. */
	const FNexusPartyState& GetPartyState() const { return PartyState; }
	
	/**
	 * Process a join request from a newly connected client.
	 * Validates party state, adds the member if accepted, rejects otherwise.
	 * Broadcasts updated state to all members on success.
	 */
	virtual void ProcessJoinRequest(ANexusPartyBeaconClient* Client, const FUniqueNetIdRepl& PlayerId, const FString& DisplayName);
	
	/**
	 * Process a voluntary leave notification from a client.
	 * Marks the member as Left, broadcasts updated state, removes the client.
	 */
	virtual void ProcessLeaveRequest(ANexusPartyBeaconClient* Client);
	
private:
	/** Push the current state snapshot to all accepted connected clients. */
	void BroadcastPartyState();
	
	/** Send a join result RPC to the requesting client. */
	void SendJoinResult(ANexusPartyBeaconClient* Client, ENexusPartyResult PartyResult);
	
	/** Find a connected + accepted client actor by player ID. */
	ANexusPartyBeaconClient* FindConnectedClient(const FUniqueNetIdRepl& PlayerId) const;
	
	/** Remove a client from ConnectedClients. Returns true if found. */
	bool RemoveConnectedClient(const ANexusPartyBeaconClient* Client);
	
private:
	/** Authoritative party state. Only valid when bPartyActive is true. */
	FNexusPartyState PartyState;
	
	/** Whether this host object has an active (initialized) party. */
	uint8 bPartyActive:1;
	
	/**
	 * All accepted party member client actors.
	 * Does NOT include the leader — the leader has no beacon client for their own host.
	 */
	UPROPERTY()
	TArray<TObjectPtr<ANexusPartyBeaconClient>> ConnectedClients;
};
