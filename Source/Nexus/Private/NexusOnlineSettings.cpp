// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusOnlineSettings.h"
#include "Beacons/NexusPingBeaconHost.h"
#include "Beacons/NexusPingBeaconClient.h"
#include "Managers/NexusBeaconManager.h"
#include "Managers/NexusFriendManager.h"
#include "Managers/NexusPartyManager.h"
#include "Managers/NexusReservationManager.h"
#include "Managers/NexusSessionManager.h"

UNexusOnlineSettings::UNexusOnlineSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Defaults
	SessionManagerClass = UNexusSessionManager::StaticClass();
	DefaultMaxPlayers = 4;
	bDefaultAllowJoinInProgress = true;
	bDefaultUsesPresence = true;
	bDefaultAllowInvites = true;
	bDefaultAllowJoinViaPresence = true;
	DefaultGameSessionName = NAME_GameSession;
	bAutoRegisterInviteHandlers = true;
	DefaultMaxSearchResults = 100;
	
	// Friends
	FriendManagerClass = UNexusFriendManager::StaticClass();
	bAutoReadFriendsList = true;
	
	// Parties
	PartyManagerClass = UNexusPartyManager::StaticClass();
	DefaultPartyMaxSize = 4;
	
	// Reservations
	ReservationManagerClass = UNexusReservationManager::StaticClass();
	bAutoStartReservationHost = false;
	ReservationListenPort = 15001;

	// Beacons
	BeaconManagerClass = UNexusBeaconManager::StaticClass();
	bAutoStartBeaconHost = true;
	BeaconListenPort = 15000;
	PingHostClass = ANexusPingBeaconHost::StaticClass();
	PingClientClass = ANexusPingBeaconClient::StaticClass();
}
