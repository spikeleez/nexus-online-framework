#include "NexusLinkSettings.h"

UNexusLinkSettings::UNexusLinkSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DefaultMaxPlayers(4)
	, bDefaultAllowJoinInProgress(true)
	, bDefaultUsesPresence(true)
	, bDefaultAllowInvites(true)
	, bDefaultAllowJoinViaPresence(true)
	, DefaultMaxSearchResults(20)
	, bAutoReadFriendsList(true)
	, DefaultGameSessionName(NAME_GameSession)
	, bAutoRegisterInviteHandlers(true)
{
	
}
