#include "NexusLinkBlueprintLibrary.h"
#include "NexusLinkSubsystem.h"
#include "NexusLinkSessionManager.h"
#include "NexusLinkFriendManager.h"
#include "NexusLinkSettings.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"

UNexusLinkSubsystem* UNexusLinkBlueprintLibrary::GetNexusLinkSubsystem(const UObject* WorldContextObject)
{
	return UNexusLinkSubsystem::Get(WorldContextObject);
}

UNexusLinkSessionManager* UNexusLinkBlueprintLibrary::GetNexusLinkSessionManager(const UObject* WorldContextObject)
{
	UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
	return Subsystem ? Subsystem->GetSessionManager() : nullptr;
}

UNexusLinkFriendManager* UNexusLinkBlueprintLibrary::GetNexusLinkFriendManager(const UObject* WorldContextObject)
{
	UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
	return Subsystem ? Subsystem->GetFriendManager() : nullptr;
}

bool UNexusLinkBlueprintLibrary::IsOnlineSubsystemAvailable()
{
	return UNexusLinkSubsystem::IsOnlineSubsystemAvailable();
}

FString UNexusLinkBlueprintLibrary::GetActiveOnlineSubsystemName()
{
	return UNexusLinkSubsystem::GetOnlineSubsystemName();
}

FUniqueNetIdRepl UNexusLinkBlueprintLibrary::GetLocalPlayerUniqueId(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return FUniqueNetIdRepl();

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World) return FUniqueNetIdRepl();

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance) return FUniqueNetIdRepl();

	ULocalPlayer* LocalPlayer = GameInstance->GetFirstGamePlayer();
	if (!LocalPlayer) return FUniqueNetIdRepl();

	return LocalPlayer->GetPreferredUniqueNetId();
}

FString UNexusLinkBlueprintLibrary::GetLocalPlayerDisplayName(const UObject* WorldContextObject)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub) return FString();

	IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
	if (!Identity.IsValid()) return FString();

	return Identity->GetPlayerNickname(0);
}

FNexusLinkHostParams UNexusLinkBlueprintLibrary::MakeDefaultHostParams()
{
	const UNexusLinkSettings* Settings = UNexusLinkSettings::Get();

	FNexusLinkHostParams Params;
	Params.MaxNumPlayers = Settings->DefaultMaxPlayers;
	Params.bAllowJoinInProgress = Settings->bDefaultAllowJoinInProgress;
	Params.bUsesPresence = Settings->bDefaultUsesPresence;
	Params.bAllowInvites = Settings->bDefaultAllowInvites;
	Params.bAllowJoinViaPresence = Settings->bDefaultAllowJoinViaPresence;
	Params.bShouldAdvertise = true;

	return Params;
}

FNexusLinkSearchParams UNexusLinkBlueprintLibrary::MakeDefaultSearchParams()
{
	const UNexusLinkSettings* Settings = UNexusLinkSettings::Get();

	FNexusLinkSearchParams Params;
	Params.MaxSearchResults = Settings->DefaultMaxSearchResults;
	Params.bSearchPresence = Settings->bDefaultUsesPresence;
	Params.bIsLanQuery = false;

	return Params;
}

FNexusLinkSessionSetting UNexusLinkBlueprintLibrary::MakeSessionSettingInt32(FName Key, int32 Value, bool bAdvertise /*= true*/)
{
	const EOnlineDataAdvertisementType::Type AdvType = bAdvertise
		? EOnlineDataAdvertisementType::ViaOnlineService
		: EOnlineDataAdvertisementType::DontAdvertise;

	return FNexusLinkSessionSetting(Key, Value, AdvType);
}

FNexusLinkSessionSetting UNexusLinkBlueprintLibrary::MakeSessionSettingString(FName Key, FString Value, bool bAdvertise /*= true*/)
{
	const EOnlineDataAdvertisementType::Type AdvType = bAdvertise
		? EOnlineDataAdvertisementType::ViaOnlineService
		: EOnlineDataAdvertisementType::DontAdvertise;

	return FNexusLinkSessionSetting(Key, Value, AdvType);
}

FNexusLinkSessionSetting UNexusLinkBlueprintLibrary::MakeSessionSettingFloat(FName Key, float Value, bool bAdvertise /*= true*/)
{
	const EOnlineDataAdvertisementType::Type AdvType = bAdvertise
		? EOnlineDataAdvertisementType::ViaOnlineService
		: EOnlineDataAdvertisementType::DontAdvertise;

	return FNexusLinkSessionSetting(Key, Value, AdvType);
}

FNexusLinkQuerySetting UNexusLinkBlueprintLibrary::MakeQuerySettingInt32(FName Key, int32 Value, ENexusLinkQueryComparisonOp Comparison)
{
	return FNexusLinkQuerySetting(Key, Value, static_cast<EOnlineComparisonOp::Type>(Comparison));
}

FNexusLinkQuerySetting UNexusLinkBlueprintLibrary::MakeQuerySettingString(FName Key, FString Value, ENexusLinkQueryComparisonOp Comparison)
{
	return FNexusLinkQuerySetting(Key, Value, static_cast<EOnlineComparisonOp::Type>(Comparison));
}

FNexusLinkQuerySetting UNexusLinkBlueprintLibrary::MakeQuerySettingFloat(FName Key, float Value, ENexusLinkQueryComparisonOp Comparison)
{
	return FNexusLinkQuerySetting(Key, Value, static_cast<EOnlineComparisonOp::Type>(Comparison));
}

bool UNexusLinkBlueprintLibrary::IsHostParamsValid(const FNexusLinkHostParams& HostParams)
{
	return HostParams.IsValid(false);
}

bool UNexusLinkBlueprintLibrary::IsSearchParamsValid(const FNexusLinkSearchParams& SearchParams)
{
	return SearchParams.IsValid(false);
}

bool UNexusLinkBlueprintLibrary::IsSearchResultValid(const FNexusLinkSearchResult& SearchResult)
{
	return SearchResult.IsValid();
}

bool UNexusLinkBlueprintLibrary::IsNamedSessionValid(const FNexusLinkNamedSession& NamedSession)
{
	return NamedSession.IsValid();
}

bool UNexusLinkBlueprintLibrary::IsOnlineFriendValid(const FNexusLinkOnlineFriend& Friend)
{
	return Friend.IsValid();
}

bool UNexusLinkBlueprintLibrary::IsPendingInviteValid(const FNexusLinkPendingInvite& Invite)
{
	return Invite.IsValid();
}

FNexusLinkSessionSettings UNexusLinkBlueprintLibrary::GetSearchResultSettings(const FNexusLinkSearchResult& SearchResult)
{
	return SearchResult.GetSessionSettings();
}

FUniqueNetIdRepl UNexusLinkBlueprintLibrary::GetSearchResultOwnerId(const FNexusLinkSearchResult& SearchResult)
{
	return SearchResult.GetOwnerUniqueId();
}

FString UNexusLinkBlueprintLibrary::GetSearchResultOwnerName(const FNexusLinkSearchResult& SearchResult)
{
	return SearchResult.GetOwnerUsername();
}

int32 UNexusLinkBlueprintLibrary::GetSearchResultPlayerCount(const FNexusLinkSearchResult& SearchResult)
{
	return SearchResult.GetNumPlayers();
}

int32 UNexusLinkBlueprintLibrary::GetSearchResultMaxPlayers(const FNexusLinkSearchResult& SearchResult)
{
	return SearchResult.GetMaxPlayers();
}

int32 UNexusLinkBlueprintLibrary::GetSearchResultOpenSlots(const FNexusLinkSearchResult& SearchResult)
{
	return SearchResult.GetNumOpenSlots();
}

int32 UNexusLinkBlueprintLibrary::GetSearchResultPing(const FNexusLinkSearchResult& SearchResult)
{
	return SearchResult.GetPing();
}

FName UNexusLinkBlueprintLibrary::GetNamedSessionName(const FNexusLinkNamedSession& NamedSession)
{
	return NamedSession.GetSessionName();
}

ENexusLinkSessionState UNexusLinkBlueprintLibrary::GetNamedSessionState(const FNexusLinkNamedSession& NamedSession)
{
	return NamedSession.GetSessionState();
}

FNexusLinkSessionSettings UNexusLinkBlueprintLibrary::GetNamedSessionSettings(const FNexusLinkNamedSession& NamedSession)
{
	return NamedSession.GetSessionSettings();
}

int32 UNexusLinkBlueprintLibrary::GetNamedSessionPlayerCount(const FNexusLinkNamedSession& NamedSession)
{
	return NamedSession.GetNumPlayers();
}

bool UNexusLinkBlueprintLibrary::GetCustomSettingInt32(const FNexusLinkSessionSettings& Settings, FName Key, int32& Value)
{
	return Settings.GetSessionSetting(Key, Value);
}

bool UNexusLinkBlueprintLibrary::GetCustomSettingString(const FNexusLinkSessionSettings& Settings, FName Key, FString& Value)
{
	return Settings.GetSessionSetting(Key, Value);
}

bool UNexusLinkBlueprintLibrary::GetCustomSettingFloat(const FNexusLinkSessionSettings& Settings, FName Key, float& Value)
{
	return Settings.GetSessionSetting(Key, Value);
}

void UNexusLinkBlueprintLibrary::ClientTravelToSession(const UObject* WorldContextObject, const FString& ConnectString)
{
	if (!WorldContextObject) return;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return;

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController) return;

	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
}
