// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NexusOnlineSettings.generated.h"

class ANexusPingBeaconHostObject;
class ANexusPingBeaconClient;

/**
 * Global configuration for the Nexus plugin.
 *
 * These settings serve as default values that can be overridden at runtime
 * through FNexusHostParams and FNexusSearchParams.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Nexus Online Settings"))
class NEXUS_API UNexusOnlineSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNexusOnlineSettings(const FObjectInitializer& ObjectInitializer);

	static const UNexusOnlineSettings* Get()
	{
		return GetDefault<UNexusOnlineSettings>();
	}

public:
	/**
	 * The default session name used for game sessions.
	 * In most cases this should remain as NAME_GameSession.
	 * Only change if you have a custom session naming convention.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Session")
	FName DefaultGameSessionName;

	/** Default maximum number of players for newly created sessions. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Session", meta = (ClampMin = "1", ClampMax = "64"))
	int32 DefaultMaxPlayers;

	/** Default: allow players to join a session already in progress. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Session")
	bool bDefaultAllowJoinInProgress;

	/**
	 * Default: use presence-based sessions (lobbies).
	 * Recommended for Steam and EOS. When true, sessions are created as lobbies
	 * which enables friend list visibility and platform overlay invites.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Session")
	bool bDefaultUsesPresence;

	/** Default: allow sending invites for this session. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Session")
	bool bDefaultAllowInvites;

	/** Default: allow joining sessions through the friends list (presence join). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Session")
	bool bDefaultAllowJoinViaPresence;

	/**
	 * Whether to automatically register handlers for incoming session invites
	 * and invite acceptance (e.g. Steam Overlay "Join Game") on subsystem init.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Session")
	bool bAutoRegisterInviteHandlers;

	/** Default maximum search results when finding sessions. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Search", meta = (ClampMin = "1", ClampMax = "100"))
	int32 DefaultMaxSearchResults;

	/**
	 * Whether to automatically read the friends list when the Nexus subsystem initializes.
	 * Disable this if you want to control when the friends list is first loaded.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Friends")
	bool bAutoReadFriendsList;

	/**
	 * When true, the beacon host starts automatically when a game session is created,
	 * and stops when the session is destroyed.
	 * Set to false if you want manual control via UNexusLinkBeaconManager::StartBeaconHost.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Beacons")
	uint8 bAutoStartBeaconHost:1;

	/**
	 * Port the beacon host listens on (server-side). Clients must use the same port when connecting.
	 * Default: 15000 (UE standard beacon port). Change only if 15000 conflicts with another service.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Beacons", meta = (ClampMin = "1024", ClampMax = "65535")) 
	int32 BeaconListenPort;

	/** A Ping Host Object class is instantiated on the server */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Beacons|Ping", meta = (AllowedClasses = "/Script/Nexus.NexusPingBeaconHostObject"))
	TSoftClassPtr<ANexusPingBeaconHostObject> PingHostObjectClass;

	/** The Ping Client class that players will instantiate. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Beacons|Ping", meta = (AllowedClasses = "/Script/Nexus.NexusPingBeaconClient"))
	TSoftClassPtr<ANexusPingBeaconClient> PingClientClass;

	/**
	 * When true, the reservation host starts automatically when a session is created,
	 * using DefaultMaxPlayers as capacity.
	 * Set to false to call StartReservationHost() manually with custom team configuration.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Beacons|Reservations")
	uint8 bAutoStartReservationHost:1;

	/**
	 * Port the reservation host listens on. Must differ from BeaconListenPort.
	 * Default: 15001.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Beacons|Reservations", meta = (ClampMin = "1024", ClampMax = "65535"))
	int32 ReservationListenPort;
};
