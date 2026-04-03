// Copyright Spike Plugins 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "OnlineBeaconClient.h"
#include "NexusPartyBeaconClient.generated.h"

/** Fired with the server's response to our join request. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FNexusClientNativeJoinResultSignature, ENexusPartyResult /*PartyResult*/, const FNexusPartyState& /*PartyState*/);

/** Fired when the server pushes an updated party state snapshot. */
DECLARE_MULTICAST_DELEGATE_OneParam(FNexusClientNativeStateUpdatedSignature, const FNexusPartyState& /*PartyState*/);

/** Fired when the local player is kicked from the party by the leader. */
DECLARE_MULTICAST_DELEGATE(FNexusClientNativeKickedSignature);

/**
 * @class ANexusPartyBeaconClient
 *
 * Client-side beacon actor for joining and maintaining presence in a remote party.
 * Connects to ANexusPartyBeaconHostObject on the party leader's AOnlineBeaconHost.
 *
 * RPC model:
 *   Client -> Server: Server RPCs declared here, executed on the server-side proxy.
 *   Server -> Client: Client RPCs declared here, called from HostObject, executed locally.
 *
 * Usage:
 *   1. Spawn via World->SpawnActor<ANexusPartyBeaconClient>().
 *   2. Call SetLocalPlayerId() and SetLocalDisplayName().
 *   3. Call ConnectToHost(HostAddress). On success, OnConnected() sends ServerRequestJoin.
 *   4. Await NativeOnJoinResult for the server's accept/reject response.
 *   5. NativeOnStateUpdated fires for subsequent party composition changes.
 */
UCLASS(BlueprintType, Blueprintable)
class NEXUS_API ANexusPartyBeaconClient : public AOnlineBeaconClient
{
	GENERATED_BODY()
	
public:
	FNexusClientNativeJoinResultSignature NativeOnJoinResultEvent;
	FNexusClientNativeStateUpdatedSignature NativeOnStateUpdatedEvent;
	FNexusClientNativeKickedSignature NativeOnKickedEvent;

public:
	ANexusPartyBeaconClient(const FObjectInitializer& ObjectInitializer);
	
	//~Begin AOnlineBeaconClient interface
	virtual void OnConnected() override;
	virtual void OnFailure() override;
	//~End of AOnlineBeaconClient interface
	
	/**
	 * Initiate a connection to the party beacon host at the given address.
	 * SetLocalPlayerId and SetLocalDisplayName MUST be called before this.
	 *
	 * @param HostAddress  IP address (or hostname) of the beacon host.
	 * @param Port         Beacon port. 0 = use BeaconListenPort from settings.
	 * @return True if the connection attempt was successfully initiated.
	 */
	virtual bool ConnectToHost(const FString& HostAddress, int32 Port = 0);
	
	/**
	 * Request to join the party. Sent automatically in OnConnected().
	 * @param PlayerId     Local player's unique net ID.
	 * @param DisplayName  Display name for the party roster.
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestJoin(const FUniqueNetIdRepl& PlayerId, const FString& DisplayName);
	
	/**
	 * Notify the host that the local player is voluntarily leaving the party.
	 * Should be sent before calling DestroyBeacon() to allow a clean state update.
	 */
	UFUNCTION(Server, Reliable)
	void ServerLeaveParty();
	
	/**
	 * Server response to our join request. Fires once per connection attempt.
	 * @param PartyResult  Accept or reject code.
	 * @param PartyState   Full party state snapshot on success, or empty state on failure.
	 */
	UFUNCTION(Client, Reliable)
	void ClientReceiveJoinResult(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState);
	
	/**
	 * Server pushes an updated party state snapshot.
	 * Fires whenever the party composition changes (join, leave, kick).
	 */
	UFUNCTION(Client, Reliable)
	void ClientReceivePartyState(const FNexusPartyState& PartyState);
	
	/**
	 * Server notifies this client that it has been kicked from the party.
	 * The beacon will be destroyed by UNexusPartyManager after this fires.
	 */
	UFUNCTION(Client, Reliable)
	void ClientReceiveKick();
	
	void SetLocalPlayerId(const FUniqueNetIdRepl& NewLocalPlayerId) { LocalPlayerId = NewLocalPlayerId; }
	void SetLocalDisplayName(const FString& NewLocalDisplayName) { LocalDisplayName = NewLocalDisplayName; }
	
	FORCEINLINE const FUniqueNetIdRepl& GetLocalPlayerId() const { return LocalPlayerId; }
	FORCEINLINE const FString& GetLocalDisplayName() const { return LocalDisplayName; }
	FORCEINLINE const FNexusPartyState& GetCachedState() const { return CachedPartyState; }
	
private:
	/** Set by the caller before connecting. Sent to the host in ServerRequestJoin. */
	FUniqueNetIdRepl LocalPlayerId;
	
	/** Display name to show on the party roster. */
	FString LocalDisplayName;
	
	/** Last received party state from the server. */
	FNexusPartyState CachedPartyState;
};
