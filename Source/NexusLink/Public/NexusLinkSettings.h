// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NexusLinkSettings.generated.h"

/**
 * Global configuration for the NexusLink plugin.
 *
 * These settings serve as default values that can be overridden at runtime
 * through FNexusLinkHostParams and FNexusLinkSearchParams.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Nexus Link Settings"))
class NEXUSLINK_API UNexusLinkSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNexusLinkSettings(const FObjectInitializer& ObjectInitializer);

	static const UNexusLinkSettings* Get()
	{
		return GetDefault<UNexusLinkSettings>();
	}

public:
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
	 * The default session name used for game sessions.
	 * In most cases this should remain as NAME_GameSession.
	 * Only change if you have a custom session naming convention.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Session")
	FName DefaultGameSessionName;

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
	 * Whether to automatically read the friends list when the NexusLink subsystem initializes.
	 * Disable this if you want to control when the friends list is first loaded.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Friends")
	bool bAutoReadFriendsList;
};
