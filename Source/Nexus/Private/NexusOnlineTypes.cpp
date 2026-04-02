// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusOnlineTypes.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "PartyBeaconState.h"

FNexuSessionSetting::FNexuSessionSetting()
	: Key(NAME_None)
	, Data(FVariantData())
	, AdvertisementType(EOnlineDataAdvertisementType::DontAdvertise)
{

}

FNexuSessionSetting::FNexuSessionSetting(const FName InKey, const FVariantData& InData, const EOnlineDataAdvertisementType::Type InAdvType)
	: Key(InKey)
	, Data(InData)
	, AdvertisementType(InAdvType)
{

}

FNexusQuerySetting::FNexusQuerySetting()
	: Key(NAME_None)
	, Data(FVariantData())
	, ComparisonOp(EOnlineComparisonOp::Equals)
{

}

FNexusQuerySetting::FNexusQuerySetting(const FName InKey, const FVariantData& InData, const EOnlineComparisonOp::Type InCompOp)
	: Key(InKey)
	, Data(InData)
	, ComparisonOp(InCompOp)
{

}

FNexusSessionSettings::FNexusSessionSettings()
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

FNexusSessionSettings::FNexusSessionSettings(const FOnlineSessionSettings& InSessionSettings)
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

FNexuHostParams::FNexuHostParams()
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
	, ExtraSessionSettings(TArray<FNexuSessionSetting>())
{

}

void FNexuHostParams::ToOnlineSessionSettings(FOnlineSessionSettings& OutSettings) const
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
	for (const FNexuSessionSetting& ExtraSetting : ExtraSessionSettings)
	{
		if (ExtraSetting.IsValid())
		{
			OutSettings.Settings.Add(ExtraSetting.Key, FOnlineSessionSetting(ExtraSetting.Data, ExtraSetting.AdvertisementType));
		}
	}
}

bool FNexuHostParams::IsValid(const bool bLogErrors /*= true*/) const
{
	bool bIsValid = true;

	if (MaxNumPlayers <= 0)
	{
		NEXUS_CLOG(bLogErrors, LogNexus, Warning, TEXT("MaxNumPlayers must be greater than zero."));
		bIsValid = false;
	}

	for (const FNexuSessionSetting& ExtraSetting : ExtraSessionSettings)
	{
		if (!ExtraSetting.IsValid())
		{
			NEXUS_CLOG(bLogErrors, LogNexus, Warning, TEXT("ExtraSessionSetting '%s' is invalid!"), *ExtraSetting.Key.ToString());
			bIsValid = false;
		}
	}

	return bIsValid;
}

FNexusSearchParams::FNexusSearchParams()
	: MaxSearchResults(20)
	, bIsLanQuery(false)
	, bSearchPresence(true)
	, ExtraQuerySettings(TArray<FNexusQuerySetting>())
	, IgnoredSessions(TArray<FUniqueNetIdRepl>())
{

}

bool FNexusSearchParams::IsValid(const bool bLogErrors /*= true*/) const
{
	bool bIsValid = true;

	if (MaxSearchResults <= 0)
	{
		NEXUS_CLOG(bLogErrors, LogNexus, Warning, TEXT("MaxSearchResults must be greater than zero."));
		bIsValid = false;
	}

	for (const FNexusQuerySetting& QuerySetting : ExtraQuerySettings)
	{
		if (!QuerySetting.IsValid())
		{
			NEXUS_CLOG(bLogErrors, LogNexus, Warning, TEXT("ExtraQuerySetting '%s' is invalid!"), *QuerySetting.Key.ToString());
			bIsValid = false;
		}
	}

	return bIsValid;
}

FNexusSearchResult::FNexusSearchResult()
	: OnlineResult(FOnlineSessionSearchResult())
{

}

FNexusSearchResult::FNexusSearchResult(const FOnlineSessionSearchResult& InResult)
	: OnlineResult(InResult)
{

}

bool FNexusSearchResult::IsValid() const
{
	return OnlineResult.IsValid();
}

FName FNexusSearchResult::GetSessionType() const
{
	// The session type is determined by the session name used during creation.
	// We default to NAME_GameSession since Nexus v1 only supports game sessions.
	return NAME_GameSession;
}

FUniqueNetIdRepl FNexusSearchResult::GetSessionUniqueId() const
{
	if (OnlineResult.Session.SessionInfo.IsValid())
	{
		return FUniqueNetIdRepl(OnlineResult.Session.SessionInfo->GetSessionId());
	}
	return FUniqueNetIdRepl();
}

FUniqueNetIdRepl FNexusSearchResult::GetOwnerUniqueId() const
{
	if (OnlineResult.Session.OwningUserId.IsValid())
	{
		return FUniqueNetIdRepl(OnlineResult.Session.OwningUserId);
	}
	return FUniqueNetIdRepl();
}

FString FNexusSearchResult::GetOwnerUsername() const
{
	return OnlineResult.Session.OwningUserName.Left(20);
}

int32 FNexusSearchResult::GetNumPlayers() const
{
	return OnlineResult.Session.SessionSettings.NumPublicConnections - OnlineResult.Session.NumOpenPublicConnections;
}

int32 FNexusSearchResult::GetNumOpenSlots() const
{
	return OnlineResult.Session.NumOpenPublicConnections;
}

int32 FNexusSearchResult::GetMaxPlayers() const
{
	return OnlineResult.Session.SessionSettings.NumPublicConnections;
}

int32 FNexusSearchResult::GetPing() const
{
	return OnlineResult.PingInMs;
}

FNexusSessionSettings FNexusSearchResult::GetSessionSettings() const
{
	return FNexusSessionSettings(OnlineResult.Session.SessionSettings);
}

FNexusOnlineFriend::FNexusOnlineFriend()
	: UserId(FUniqueNetIdRepl())
	, DisplayName(FString())
	, Presence(ENexusFriendPresence::Offline)
	, bIsOnline(false)
	, bIsInGame(false)
{

}

FNexusOnlineFriend::FNexusOnlineFriend(const FOnlineFriend& NativeFriend)
{
	UserId = FUniqueNetIdRepl(NativeFriend.GetUserId());
	DisplayName = NativeFriend.GetDisplayName();

	const FOnlineUserPresence& PresenceInfo = NativeFriend.GetPresence();
	bIsOnline = PresenceInfo.bIsOnline;
	bIsInGame = PresenceInfo.bIsPlayingThisGame;

	// Map to our presence enum.
	if (!bIsOnline)
	{
		Presence = ENexusFriendPresence::Offline;
	}
	else if (bIsInGame)
	{
		Presence = ENexusFriendPresence::InGame;
	}
	else
	{
		Presence = ENexusFriendPresence::Online;
	}
}

FPartyReservation FNexusPartyReservation::ToNative() const
{
	FPartyReservation Native;
	Native.PartyLeader = PartyLeader.ToNative().UniqueId;

	Native.PartyMembers.Reserve(PartyMembers.Num());
	for (const FNexusPartyMember& Member : PartyMembers)
	{
		Native.PartyMembers.Add(Member.ToNative());
	}

	Native.TeamNum = TeamNum;
	return Native;
}

FPlayerReservation FNexusPartyMember::ToNative() const
{
	FPlayerReservation Native;
	Native.UniqueId = UniqueId;
	Native.Platform = Platform;
	Native.ValidationStr = ValidationStr;
	Native.bAllowCrossplay = bAllowCrossPlay;
	Native.ElapsedTime = 0.0f;
	return Native;
}
