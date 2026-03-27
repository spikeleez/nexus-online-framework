// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusLinkTypes.h"
#include "Interfaces/OnlinePresenceInterface.h"

FNexusLinkSessionSetting::FNexusLinkSessionSetting()
	: Key(NAME_None)
	, Data(FVariantData())
	, AdvertisementType(EOnlineDataAdvertisementType::DontAdvertise)
{

}

FNexusLinkSessionSetting::FNexusLinkSessionSetting(const FName InKey, const FVariantData& InData, const EOnlineDataAdvertisementType::Type InAdvType)
	: Key(InKey)
	, Data(InData)
	, AdvertisementType(InAdvType)
{

}

FNexusLinkQuerySetting::FNexusLinkQuerySetting()
	: Key(NAME_None)
	, Data(FVariantData())
	, ComparisonOp(EOnlineComparisonOp::Equals)
{

}

FNexusLinkQuerySetting::FNexusLinkQuerySetting(const FName InKey, const FVariantData& InData, const EOnlineComparisonOp::Type InCompOp)
	: Key(InKey)
	, Data(InData)
	, ComparisonOp(InCompOp)
{

}

FNexusLinkSessionSettings::FNexusLinkSessionSettings()
	: SessionSettings(FOnlineSessionSettings())
	, ServerName(FString())
	, MapName(FString())
	, GameMode(FString())
	, MaxNumPlayers(0)
	, bShouldAdvertise(false)
	, bHidden(false)
	, bAllowJoinInProgress(false)
	, bIsLanMatch(false)
	, bUsesPresence(false)
	, bAllowInvites(false)
	, bAllowJoinViaPresence(false)
{

}

FNexusLinkSessionSettings::FNexusLinkSessionSettings(const FOnlineSessionSettings& InSessionSettings)
{
	SessionSettings = InSessionSettings;
	MaxNumPlayers = InSessionSettings.NumPublicConnections;
	bShouldAdvertise = InSessionSettings.bShouldAdvertise;
	bAllowJoinInProgress = InSessionSettings.bAllowJoinInProgress;
	bIsLanMatch = InSessionSettings.bIsLANMatch;
	bUsesPresence = InSessionSettings.bUsesPresence;
	bAllowInvites = InSessionSettings.bAllowInvites;
	bAllowJoinViaPresence = InSessionSettings.bAllowJoinViaPresence;

	// Initialize string fields to empty before reading.
	ServerName = FString();
	MapName = FString();
	GameMode = FString();
	bHidden = false;

	// Read custom session settings.
	InSessionSettings.Get(NEXUS_SERVERNAME, ServerName);
	InSessionSettings.Get(NEXUS_MAPNAME, MapName);
	InSessionSettings.Get(NEXUS_GAMEMODE, GameMode);

	// Hidden flag is stored as int32 because Steam OSS doesn't support bool queries.
	int32 HiddenValue = 0;
	InSessionSettings.Get(NEXUS_HIDDEN, HiddenValue);
	bHidden = HiddenValue != 0;
}

FNexusLinkHostParams::FNexusLinkHostParams()
	: StartingLevel(FString())
	, ServerName(FString())
	, MapName(FString())
	, GameMode(FString())
	, MaxNumPlayers(4)
	, bShouldAdvertise(true)
	, bHidden(false)
	, bAllowJoinInProgress(true)
	, bIsLanMatch(false)
	, bUsesPresence(true)
	, bAllowInvites(true)
	, bAllowJoinViaPresence(true)
	, ExtraSessionSettings(TArray<FNexusLinkSessionSetting>())
{

}

void FNexusLinkHostParams::ToOnlineSessionSettings(FOnlineSessionSettings& OutSettings) const
{
	// If we have a full override, use it directly and return.
	if (SessionSettingsOverride.IsSet())
	{
		OutSettings = SessionSettingsOverride.GetValue();
		return;
	}

	// Build from individual fields.
	OutSettings.NumPublicConnections = MaxNumPlayers;
	OutSettings.NumPrivateConnections = 0;
	OutSettings.bShouldAdvertise = bShouldAdvertise;
	OutSettings.bAllowJoinInProgress = bAllowJoinInProgress;
	OutSettings.bIsLANMatch = bIsLanMatch;
	OutSettings.bUsesPresence = bUsesPresence;
	OutSettings.bAllowInvites = bAllowInvites;
	OutSettings.bAllowJoinViaPresence = bAllowJoinViaPresence;
	OutSettings.bUseLobbiesIfAvailable = bUsesPresence;

	// Apply custom string settings.
	if (!ServerName.IsEmpty())
	{
		OutSettings.Set(NEXUS_SERVERNAME, ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	}

	if (!MapName.IsEmpty())
	{
		OutSettings.Set(NEXUS_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	}

	if (!GameMode.IsEmpty())
	{
		OutSettings.Set(NEXUS_GAMEMODE, GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	}

	// Hidden flag stored as int32 for Steam compatibility.
	const int32 HiddenValue = bHidden ? 1 : 0;
	OutSettings.Set(NEXUS_HIDDEN, HiddenValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// Apply extra settings using FVariantData directly.
	for (const FNexusLinkSessionSetting& ExtraSetting : ExtraSessionSettings)
	{
		if (ExtraSetting.IsValid())
		{
			OutSettings.Settings.Add(ExtraSetting.Key, FOnlineSessionSetting(ExtraSetting.Data, ExtraSetting.AdvertisementType));
		}
	}
}

bool FNexusLinkHostParams::IsValid(const bool bLogErrors /*= true*/) const
{
	bool bIsValid = true;

	if (MaxNumPlayers <= 0)
	{
		NEXUS_CLOG(bLogErrors, LogNexusLink, Warning, TEXT("MaxNumPlayers must be greater than zero."));
		bIsValid = false;
	}

	for (const FNexusLinkSessionSetting& ExtraSetting : ExtraSessionSettings)
	{
		if (!ExtraSetting.IsValid())
		{
			NEXUS_CLOG(bLogErrors, LogNexusLink, Warning, TEXT("ExtraSessionSetting '%s' is invalid!"), *ExtraSetting.Key.ToString());
			bIsValid = false;
		}
	}

	return bIsValid;
}

FNexusLinkSearchParams::FNexusLinkSearchParams()
	: MaxSearchResults(20)
	, bIsLanQuery(false)
	, bSearchPresence(true)
	, ExtraQuerySettings(TArray<FNexusLinkQuerySetting>())
	, IgnoredSessions(TArray<FUniqueNetIdRepl>())
{

}

bool FNexusLinkSearchParams::IsValid(const bool bLogErrors /*= true*/) const
{
	bool bIsValid = true;

	if (MaxSearchResults <= 0)
	{
		NEXUS_CLOG(bLogErrors, LogNexusLink, Warning, TEXT("MaxSearchResults must be greater than zero."));
		bIsValid = false;
	}

	for (const FNexusLinkQuerySetting& QuerySetting : ExtraQuerySettings)
	{
		if (!QuerySetting.IsValid())
		{
			NEXUS_CLOG(bLogErrors, LogNexusLink, Warning, TEXT("ExtraQuerySetting '%s' is invalid!"), *QuerySetting.Key.ToString());
			bIsValid = false;
		}
	}

	return bIsValid;
}

FNexusLinkSearchResult::FNexusLinkSearchResult()
	: OnlineResult(FOnlineSessionSearchResult())
{

}

FNexusLinkSearchResult::FNexusLinkSearchResult(const FOnlineSessionSearchResult& InResult)
	: OnlineResult(InResult)
{

}

bool FNexusLinkSearchResult::IsValid() const
{
	return OnlineResult.IsValid();
}

FName FNexusLinkSearchResult::GetSessionType() const
{
	// The session type is determined by the session name used during creation.
	// We default to NAME_GameSession since NexusLink v1 only supports game sessions.
	return NAME_GameSession;
}

FUniqueNetIdRepl FNexusLinkSearchResult::GetSessionUniqueId() const
{
	if (OnlineResult.Session.SessionInfo.IsValid())
	{
		return FUniqueNetIdRepl(OnlineResult.Session.SessionInfo->GetSessionId());
	}
	return FUniqueNetIdRepl();
}

FUniqueNetIdRepl FNexusLinkSearchResult::GetOwnerUniqueId() const
{
	if (OnlineResult.Session.OwningUserId.IsValid())
	{
		return FUniqueNetIdRepl(OnlineResult.Session.OwningUserId);
	}
	return FUniqueNetIdRepl();
}

FString FNexusLinkSearchResult::GetOwnerUsername() const
{
	return OnlineResult.Session.OwningUserName.Left(20);
}

int32 FNexusLinkSearchResult::GetNumPlayers() const
{
	return OnlineResult.Session.SessionSettings.NumPublicConnections - OnlineResult.Session.NumOpenPublicConnections;
}

int32 FNexusLinkSearchResult::GetNumOpenSlots() const
{
	return OnlineResult.Session.NumOpenPublicConnections;
}

int32 FNexusLinkSearchResult::GetMaxPlayers() const
{
	return OnlineResult.Session.SessionSettings.NumPublicConnections;
}

int32 FNexusLinkSearchResult::GetPing() const
{
	return OnlineResult.PingInMs;
}

FNexusLinkSessionSettings FNexusLinkSearchResult::GetSessionSettings() const
{
	return FNexusLinkSessionSettings(OnlineResult.Session.SessionSettings);
}

FNexusLinkOnlineFriend::FNexusLinkOnlineFriend()
	: UserId(FUniqueNetIdRepl())
	, DisplayName(FString())
	, Presence(ENexusLinkFriendPresence::Offline)
	, bIsOnline(false)
	, bIsInGame(false)
{

}

FNexusLinkOnlineFriend::FNexusLinkOnlineFriend(const FOnlineFriend& NativeFriend)
{
	UserId = FUniqueNetIdRepl(NativeFriend.GetUserId());
	DisplayName = NativeFriend.GetDisplayName();

	const FOnlineUserPresence& PresenceInfo = NativeFriend.GetPresence();
	bIsOnline = PresenceInfo.bIsOnline;
	bIsInGame = PresenceInfo.bIsPlayingThisGame;

	// Map to our presence enum.
	if (!bIsOnline)
	{
		Presence = ENexusLinkFriendPresence::Offline;
	}
	else if (bIsInGame)
	{
		Presence = ENexusLinkFriendPresence::InGame;
	}
	else
	{
		Presence = ENexusLinkFriendPresence::Online;
	}
}
