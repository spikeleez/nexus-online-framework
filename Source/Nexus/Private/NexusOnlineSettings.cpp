// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusOnlineSettings.h"
#include "Beacons/NexusPingBeaconHostObject.h"
#include "Beacons/NexusPingBeaconClient.h"

UNexusOnlineSettings::UNexusOnlineSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Defaults
	DefaultMaxPlayers = 4;
	bDefaultAllowJoinInProgress = true;
	bDefaultUsesPresence = true;
	bDefaultAllowInvites = true;
	bDefaultAllowJoinViaPresence = true;
	DefaultGameSessionName = NAME_GameSession;
	bAutoRegisterInviteHandlers = true;
	DefaultMaxSearchResults = 100;
	bAutoReadFriendsList = true;

	// Beacons
	bAutoStartBeaconHost = true;
	BeaconListenPort = 15000;
	bAutoStartReservationHost = false;
	ReservationListenPort = 15001;
	PingHostObjectClass = ANexusPingBeaconHostObject::StaticClass();
	PingClientClass = ANexusPingBeaconClient::StaticClass();
}
