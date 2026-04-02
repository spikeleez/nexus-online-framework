// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusOnlineLibrary.h"
#include "NexusOnlineSubsystem.h"
#include "NexusOnlineSettings.h"
#include "Managers/NexusSessionManager.h"
#include "Managers/NexusFriendManager.h"
#include "Managers/NexusBeaconManager.h"
#include "Managers/NexusReservationManager.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

UNexusSessionManager* UNexusOnlineLibrary::GetNexusSessionManager(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult)
{
	UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	UNexusSessionManager* SessionManager = Subsystem->GetSessionManager();
	if (!SessionManager)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return SessionManager;
}

UNexusFriendManager* UNexusOnlineLibrary::GetNexusFriendManager(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult)
{
	UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	UNexusFriendManager* FriendManager = Subsystem->GetFriendManager();
	if (!FriendManager)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return FriendManager;
}

bool UNexusOnlineLibrary::IsOnlineSubsystemAvailable()
{
	return UNexusOnlineSubsystem::IsOnlineSubsystemAvailable();
}

FString UNexusOnlineLibrary::GetActiveOnlineSubsystemName()
{
	return UNexusOnlineSubsystem::GetOnlineSubsystemName();
}

FUniqueNetIdRepl UNexusOnlineLibrary::GetLocalPlayerUniqueId(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult)
{
	if (!WorldContextObject)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return FUniqueNetIdRepl();
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return FUniqueNetIdRepl();
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return FUniqueNetIdRepl();
	}

	ULocalPlayer* LocalPlayer = GameInstance->GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return FUniqueNetIdRepl();
	}

	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return LocalPlayer->GetPreferredUniqueNetId();
}

FString UNexusOnlineLibrary::GetLocalPlayerDisplayName(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return FString();
	}

	IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return FString();
	}

	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return Identity->GetPlayerNickname(0);
}

UNexusBeaconManager* UNexusOnlineLibrary::GetBeaconManager(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult)
{
	UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	UNexusBeaconManager* BeaconManager = Subsystem->GetBeaconManager();
	if (!BeaconManager)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return BeaconManager;
}

UNexusReservationManager* UNexusOnlineLibrary::GetReservationManager(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult)
{
	UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	UNexusReservationManager* ReservationManager = Subsystem->GetReservationManager();
	if (!ReservationManager)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return ReservationManager;
}

FNexuHostParams UNexusOnlineLibrary::MakeDefaultHostParams()
{
	const UNexusOnlineSettings* Settings = UNexusOnlineSettings::Get();

	FNexuHostParams Params;
	Params.MaxNumPlayers = Settings->DefaultMaxPlayers;
	Params.bAllowJoinInProgress = Settings->bDefaultAllowJoinInProgress;
	Params.bUsesPresence = Settings->bDefaultUsesPresence;
	Params.bAllowInvites = Settings->bDefaultAllowInvites;
	Params.bAllowJoinViaPresence = Settings->bDefaultAllowJoinViaPresence;
	Params.bShouldAdvertise = true;

	return Params;
}

FNexusSearchParams UNexusOnlineLibrary::MakeDefaultSearchParams()
{
	const UNexusOnlineSettings* Settings = UNexusOnlineSettings::Get();

	FNexusSearchParams Params;
	Params.MaxSearchResults = Settings->DefaultMaxSearchResults;
	Params.bSearchPresence = Settings->bDefaultUsesPresence;
	Params.bIsLanQuery = false;

	return Params;
}

FNexuSessionSetting UNexusOnlineLibrary::MakeSessionSettingInt32(FName Key, int32 Value, bool bAdvertise /*= true*/)
{
	const EOnlineDataAdvertisementType::Type AdvType = bAdvertise
		? EOnlineDataAdvertisementType::ViaOnlineService
		: EOnlineDataAdvertisementType::DontAdvertise;

	return FNexuSessionSetting(Key, Value, AdvType);
}

FNexuSessionSetting UNexusOnlineLibrary::MakeSessionSettingString(FName Key, FString Value, bool bAdvertise /*= true*/)
{
	const EOnlineDataAdvertisementType::Type AdvType = bAdvertise
		? EOnlineDataAdvertisementType::ViaOnlineService
		: EOnlineDataAdvertisementType::DontAdvertise;

	return FNexuSessionSetting(Key, Value, AdvType);
}

FNexuSessionSetting UNexusOnlineLibrary::MakeSessionSettingFloat(FName Key, float Value, bool bAdvertise /*= true*/)
{
	const EOnlineDataAdvertisementType::Type AdvType = bAdvertise
		? EOnlineDataAdvertisementType::ViaOnlineService
		: EOnlineDataAdvertisementType::DontAdvertise;

	return FNexuSessionSetting(Key, Value, AdvType);
}

FNexusQuerySetting UNexusOnlineLibrary::MakeQuerySettingInt32(FName Key, int32 Value, ENexuQueryComparisonOp Comparison)
{
	return FNexusQuerySetting(Key, Value, static_cast<EOnlineComparisonOp::Type>(Comparison));
}

FNexusQuerySetting UNexusOnlineLibrary::MakeQuerySettingString(FName Key, FString Value, ENexuQueryComparisonOp Comparison)
{
	return FNexusQuerySetting(Key, Value, static_cast<EOnlineComparisonOp::Type>(Comparison));
}

FNexusQuerySetting UNexusOnlineLibrary::MakeQuerySettingFloat(FName Key, float Value, ENexuQueryComparisonOp Comparison)
{
	return FNexusQuerySetting(Key, Value, static_cast<EOnlineComparisonOp::Type>(Comparison));
}

bool UNexusOnlineLibrary::IsHostParamsValid(const FNexuHostParams& HostParams)
{
	return HostParams.IsValid(false);
}

bool UNexusOnlineLibrary::IsSearchParamsValid(const FNexusSearchParams& SearchParams)
{
	return SearchParams.IsValid(false);
}

bool UNexusOnlineLibrary::IsSearchResultValid(const FNexusSearchResult& SearchResult)
{
	return SearchResult.IsValid();
}

bool UNexusOnlineLibrary::IsNamedSessionValid(const FNexusNamedSession& NamedSession)
{
	return NamedSession.IsValid();
}

bool UNexusOnlineLibrary::IsOnlineFriendValid(const FNexusOnlineFriend& Friend)
{
	return Friend.IsValid();
}

bool UNexusOnlineLibrary::IsPendingInviteValid(const FNexusPendingInvite& Invite)
{
	return Invite.IsValid();
}

FNexusSessionSettings UNexusOnlineLibrary::GetSearchResultSettings(const FNexusSearchResult& SearchResult)
{
	return SearchResult.GetSessionSettings();
}

FUniqueNetIdRepl UNexusOnlineLibrary::GetSearchResultOwnerId(const FNexusSearchResult& SearchResult)
{
	return SearchResult.GetOwnerUniqueId();
}

FString UNexusOnlineLibrary::GetSearchResultOwnerName(const FNexusSearchResult& SearchResult)
{
	return SearchResult.GetOwnerUsername();
}

int32 UNexusOnlineLibrary::GetSearchResultPlayerCount(const FNexusSearchResult& SearchResult)
{
	return SearchResult.GetNumPlayers();
}

int32 UNexusOnlineLibrary::GetSearchResultMaxPlayers(const FNexusSearchResult& SearchResult)
{
	return SearchResult.GetMaxPlayers();
}

int32 UNexusOnlineLibrary::GetSearchResultOpenSlots(const FNexusSearchResult& SearchResult)
{
	return SearchResult.GetNumOpenSlots();
}

int32 UNexusOnlineLibrary::GetSearchResultPing(const FNexusSearchResult& SearchResult)
{
	return SearchResult.GetPing();
}

FName UNexusOnlineLibrary::GetNamedSessionName(const FNexusNamedSession& NamedSession)
{
	return NamedSession.GetSessionName();
}

ENexusSessionState UNexusOnlineLibrary::GetNamedSessionState(const FNexusNamedSession& NamedSession)
{
	return NamedSession.GetSessionState();
}

FNexusSessionSettings UNexusOnlineLibrary::GetNamedSessionSettings(const FNexusNamedSession& NamedSession)
{
	return NamedSession.GetSessionSettings();
}

int32 UNexusOnlineLibrary::GetNamedSessionPlayerCount(const FNexusNamedSession& NamedSession)
{
	return NamedSession.GetNumPlayers();
}

bool UNexusOnlineLibrary::GetCustomSettingInt32(const FNexusSessionSettings& Settings, FName Key, int32& Value)
{
	return Settings.GetSessionSetting(Key, Value);
}

bool UNexusOnlineLibrary::GetCustomSettingString(const FNexusSessionSettings& Settings, FName Key, FString& Value)
{
	return Settings.GetSessionSetting(Key, Value);
}

bool UNexusOnlineLibrary::GetCustomSettingFloat(const FNexusSessionSettings& Settings, FName Key, float& Value)
{
	return Settings.GetSessionSetting(Key, Value);
}

void UNexusOnlineLibrary::ClientTravelToSession(const UObject* WorldContextObject, const FString& ConnectString)
{
	if (!WorldContextObject) return;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return;

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController) return;

	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
}
