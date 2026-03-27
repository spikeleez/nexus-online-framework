// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusLinkSettings.h"

UNexusLinkSettings::UNexusLinkSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DefaultMaxPlayers(4)
	, bDefaultAllowJoinInProgress(true)
	, bDefaultUsesPresence(true)
	, bDefaultAllowInvites(true)
	, bDefaultAllowJoinViaPresence(true)
	, DefaultGameSessionName(NAME_GameSession)
	, bAutoRegisterInviteHandlers(true)
	, DefaultMaxSearchResults(100)
	, bAutoReadFriendsList(true)
{
	
}
