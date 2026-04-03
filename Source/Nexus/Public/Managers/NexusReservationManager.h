#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "NexusReservationManager.generated.h"

class APartyBeaconHost;
class APartyBeaconClient;
class UGameInstance;

namespace EPartyReservationResult { enum Type : int; }

/** A player/party's reservation was accepted. */
DECLARE_MULTICAST_DELEGATE_OneParam(FNexusOnReservationAcceptedSignature, const FNexusReservationSummary&);

/** A reservation was cancelled by the client or timed out. */
DECLARE_MULTICAST_DELEGATE_OneParam(FNexusOnReservationCancelledSignature, const FUniqueNetIdRepl& /*PartyLeaderId*/);

/** All slots are now reserved — no more requests will be accepted. */
DECLARE_MULTICAST_DELEGATE(FNexusOnReservationsFullSignature);

/** Fired when the server responds to our reservation request. */
DECLARE_MULTICAST_DELEGATE_OneParam(FNexusOnReservationResponseSignature, ENexusReservationResult);

/**
 * @class UNexusReservationManager
 * 
 * Wraps UE5's APartyBeaconHost / APartyBeaconClient system.
 * Owned by UNexusOnlineSubsystem.
 */
UCLASS()
class NEXUS_API UNexusReservationManager : public UObject
{
	GENERATED_BODY()

public:
	/** [Server] Fired when a reservation request is accepted. */
	FNexusOnReservationAcceptedSignature NativeOnReservationAccepted;

	/** [Server] Fired when a reservation is cancelled. */
	FNexusOnReservationCancelledSignature NativeOnReservationCancelled;

	/** [Server] Fired when all slots are filled. Ideal trigger to start the game. */
	FNexusOnReservationsFullSignature NativeOnReservationsFull;

	/** [Client] Fired with the server's response to our reservation request. */
	FNexusOnReservationResponseSignature NativeOnReservationResponse;

public:
	UNexusReservationManager(const FObjectInitializer& ObjectInitializer);

	//~Begin UObject interface
	virtual UWorld* GetWorld() const override;
	//~End of UObject interface

	void Initialize(UGameInstance* InGameInstance);
	void Deinitialize();

	/** Start accepting reservation requests. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Reservations")
	bool StartReservationHost(FName SessionName, int32 MaxReservations, int32 TeamCount = 1, int32 TeamSize = 0);

	/** Tear down the reservation host. All pending reservations are cleared. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Reservations")
	void StopReservationHost();

	/** @return Whether the reservation host is currently active. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Reservations")
	bool IsReservationHostActive() const { return bReservationHostActive; }

	/** How many reservation slots remain. Returns 0 if the host is not active. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Reservations")
	int32 GetNumRemainingReservations() const;

	/** How many players have successfully reserved. Returns 0 if the host is not active. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Reservations")
	int32 GetNumPlayersReserved() const;

	/**
	 * Send a reservation request for the given party via an OSS search result.
	 *
	 * This is the primary client-side API. Prefer it over the raw address variant
	 * whenever you have a FNexusLinkSearchResult (Steam, EOS, etc.).
	 *
	 * The spawned APartyBeaconClient manages its own lifetime — do not store
	 * the returned pointer beyond the current frame.
	 *
	 * Bind NativeOnReservationResponse before or immediately after calling this.
	 * 
	 * @param SearchResult The session to request a reservation in.
	 * @param Reservation Your party's data.
	 * @return The spawned client actor, or nullptr on failure.
	 */
	APartyBeaconClient* RequestReservation(const FNexusSearchResult& SearchResult, const FNexusPartyReservation& Reservation);

	/**
	 * Send a reservation request via a raw address string.
	 * Used for LAN / dedicated server setups without an OSS search result.
	 *
	 * @param HostAddress "IP:Port" — port must match ReservationListenPort in settings.
	 * @param SessionId Session identifier string from the host's online session.
	 * @param Reservation Your party's data.
	 * @return The spawned client actor, or nullptr on failure.
	 */
	APartyBeaconClient* RequestReservationDirect(const FString& HostAddress, const FString& SessionId, const FNexusPartyReservation& Reservation);

	/**
	 * Cancel a previously sent reservation request.
	 * Only valid on the client that originally sent the request.
	 *
	 * @param PartyLeaderId  Must match the leader ID from the original request.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Reservations")
	void CancelReservation(const FUniqueNetIdRepl& PartyLeaderId);

	virtual void OnSessionCreated(ENexusCreateSessionResult Result);
	virtual void OnSessionDestroyed(ENexusDestroySessionResult Result);

private:
	/** Called by APartyBeaconHost when any reservation changes (add, cancel, full). */
	void OnReservationChanged();

	/** Called by APartyBeaconHost when all slots are full. */
	void OnReservationsFull();

	/** Called by APartyBeaconHost when a new player is added to a reservation. */
	void OnNewPlayerAdded(const FPlayerReservation& NewPlayer);

	/** Called by APartyBeaconHost when a reservation is cancelled. */
	void OnCancelledReservation(const FUniqueNetId& PartyLeaderId);

	/** Called by APartyBeaconClient with the server's response. */
	void OnReservationRequestComplete(EPartyReservationResult::Type NativeResult);

	/** Convert EPartyReservationResult (engine) -> ENexusReservationResult. */
	static ENexusReservationResult ConvertReservationResult(EPartyReservationResult::Type NativeResult);

	/** Attempt to rebuild a FNexusReservationSummary for a given leader from host state. */
	FNexusReservationSummary BuildSummaryForLeader(const FUniqueNetIdRepl& LeaderId) const;

private:
	/** The server-side party beacon host. Null on clients. */
	UPROPERTY()
	TObjectPtr<APartyBeaconHost> ReservationHost;

	/**
	 * The active client-side beacon actor. Null when not in the middle of a request.
	 * One active request at a time — cancel before sending a new one.
	 */
	UPROPERTY()
	TObjectPtr<APartyBeaconClient> ActiveReservationClient;

	TWeakObjectPtr<UGameInstance> CachedGameInstance;

	uint8 bReservationHostActive:1;

	/** Cached from the last StartReservationHost call, used for auto-restart. */
	FName CachedSessionName;
	int32 CachedMaxReservations;
	int32 CachedTeamCount;
	int32 CachedTeamSize;
};
