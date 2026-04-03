// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NexusOnlineTypes.h"
#include "NexusOnlineLibrary.generated.h"

class UNexusPartyManager;
class UNexusOnlineSubsystem;
class UNexusSessionManager;
class UNexusFriendManager;
class UNexusBeaconManager;
class UNexusReservationManager;

/**
 * @class UNexusOnlineLibrary
 * 
 * Static Blueprint function library for Nexus.
 * Provides convenience accessors, factory functions, and utility helpers
 * that can be called from any Blueprint without needing a direct reference
 * to the subsystem or its managers.
 */
UCLASS()
class NEXUS_API UNexusOnlineLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** Get the session manager from any world context. */
	UFUNCTION(Category = "Nexus|Session", BlueprintCallable, meta = (WorldContext = "WorldContextObject", ExpandEnumAsExecs = "OutResult"))
	static UNexusSessionManager* GetNexusSessionManager(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult);

	/** Create a FNexusHostParams populated with defaults from Plugin Settings. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure)
	static FNexuHostParams MakeDefaultHostParams();

	/** Create a FNexusSearchParams populated with defaults from Plugin Settings. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure)
	static FNexusSearchParams MakeDefaultSearchParams();
	
	/** @return Whether the host params are valid. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsHostParamsValid(const FNexuHostParams& HostParams);

	/** @return Whether the search params are valid. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsSearchParamsValid(const FNexusSearchParams& SearchParams);

	/** @return Whether the search result is valid. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsSearchResultValid(const FNexusSearchResult& SearchResult);

	/** @return Whether the named session is valid. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsNamedSessionValid(const FNexusNamedSession& NamedSession);

	/** @return The session settings from a search result. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static FNexusSessionSettings GetSearchResultSettings(const FNexusSearchResult& SearchResult);

	/** @return The owner's unique ID from a search result. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static FUniqueNetIdRepl GetSearchResultOwnerId(const FNexusSearchResult& SearchResult);

	/** @return The owner's display name from a search result. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static FString GetSearchResultOwnerName(const FNexusSearchResult& SearchResult);

	/** @return Current player count from a search result. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static int32 GetSearchResultPlayerCount(const FNexusSearchResult& SearchResult);

	/** @return Max player count from a search result. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static int32 GetSearchResultMaxPlayers(const FNexusSearchResult& SearchResult);

	/** @return Open slots from a search result. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static int32 GetSearchResultOpenSlots(const FNexusSearchResult& SearchResult);

	/** @return Ping from a search result. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static int32 GetSearchResultPing(const FNexusSearchResult& SearchResult);

	/** @return The session name from a named session. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static FName GetNamedSessionName(const FNexusNamedSession& NamedSession);

	/** @return The session state from a named session. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static ENexusSessionState GetNamedSessionState(const FNexusNamedSession& NamedSession);

	/** @return The session settings from a named session. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static FNexusSessionSettings GetNamedSessionSettings(const FNexusNamedSession& NamedSession);

	/** @return Player count from a named session. */
	UFUNCTION(Category = "Nexus|Session", BlueprintPure, meta = (ScriptMethod))
	static int32 GetNamedSessionPlayerCount(const FNexusNamedSession& NamedSession);

	UFUNCTION(Category = "Nexus|Session", BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void ClientTravelToSession(const UObject* WorldContextObject, const FString& ConnectString);

	/** Create a session setting from an int32 value. */
	UFUNCTION(Category = "Nexus|Session|Settings", BlueprintPure, DisplayName = "Make Session Setting (int32)", meta = (NativeMakeFunc))
	static FNexuSessionSetting MakeSessionSettingInt32(FName Key, int32 Value, bool bAdvertise = true);

	/** Create a session setting from a string value. */
	UFUNCTION(Category = "Nexus|Session|Settings", BlueprintPure, DisplayName = "Make Session Setting (String)", meta = (NativeMakeFunc))
	static FNexuSessionSetting MakeSessionSettingString(FName Key, FString Value, bool bAdvertise = true);

	/** Create a session setting from a float value. */
	UFUNCTION(Category = "Nexus|Session|Settings", BlueprintPure, DisplayName = "Make Session Setting (float)", meta = (NativeMakeFunc))
	static FNexuSessionSetting MakeSessionSettingFloat(FName Key, float Value, bool bAdvertise = true);

	/** Create a query setting from an int32 value. */
	UFUNCTION(Category = "Nexus|Session|Settings", BlueprintPure, DisplayName = "Make Query Setting (int32)", meta = (NativeMakeFunc))
	static FNexusQuerySetting MakeQuerySettingInt32(FName Key, int32 Value, ENexuQueryComparisonOp Comparison);

	/** Create a query setting from a string value. */
	UFUNCTION(Category = "Nexus|Session|Settings", BlueprintPure, DisplayName = "Make Query Setting (String)", meta = (NativeMakeFunc))
	static FNexusQuerySetting MakeQuerySettingString(FName Key, FString Value, ENexuQueryComparisonOp Comparison);

	/** Create a query setting from a float value. */
	UFUNCTION(Category = "Nexus|Session|Settings", BlueprintPure, DisplayName = "Make Query Setting (float)", meta = (NativeMakeFunc))
	static FNexusQuerySetting MakeQuerySettingFloat(FName Key, float Value, ENexuQueryComparisonOp Comparison);

	/** Get a custom int32 setting from session settings. */
	UFUNCTION(Category = "Nexus|Session|Settings", BlueprintPure, DisplayName = "Get Custom Setting (int32)", meta = (ReturnDisplayName = "Success", AutoCreateRefTerm = "Value"))
	static bool GetCustomSettingInt32(const FNexusSessionSettings& Settings, FName Key, int32& Value);

	/** Get a custom string setting from session settings. */
	UFUNCTION(Category = "Nexus|Session|Settings", BlueprintPure, DisplayName = "Get Custom Setting (String)", meta = (ReturnDisplayName = "Success", AutoCreateRefTerm = "Value"))
	static bool GetCustomSettingString(const FNexusSessionSettings& Settings, FName Key, FString& Value);

	/** Get a custom float setting from session settings. */
	UFUNCTION(Category = "Nexus|Session|Settings", BlueprintPure, DisplayName = "Get Custom Setting (float)", meta = (ReturnDisplayName = "Success", AutoCreateRefTerm = "Value"))
	static bool GetCustomSettingFloat(const FNexusSessionSettings& Settings, FName Key, float& Value);

	/** Get the friend manager from any world context. */
	UFUNCTION(Category = "Nexus|Friends", BlueprintCallable, meta = (WorldContext = "WorldContextObject", ExpandEnumAsExecs = "OutResult"))
	static UNexusFriendManager* GetNexusFriendManager(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult);

	/** @return Whether the online friend is valid. */
	UFUNCTION(Category = "Nexus|Friends", BlueprintPure, DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsOnlineFriendValid(const FNexusOnlineFriend& Friend);

	/** @return Whether the pending invite is valid. */
	UFUNCTION(Category = "Nexus|Friends", BlueprintPure, DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsPendingInviteValid(const FNexusPendingInvite& Invite);

	/** @return Whether any online subsystem is available. */
	UFUNCTION(Category = "Nexus|Utility", BlueprintPure)
	static bool IsOnlineSubsystemAvailable();

	/** @return Name of the active online subsystem (e.g. "Steam", "EOS", "NULL"). */
	UFUNCTION(Category = "Nexus|Utility", BlueprintPure)
	static FString GetActiveOnlineSubsystemName();

	/** @return The local player's unique net ID from the online subsystem. */
	UFUNCTION(Category = "Nexus|Utility", BlueprintCallable, meta = (WorldContext = "WorldContextObject", ExpandEnumAsExecs = "OutResult"))
	static FUniqueNetIdRepl GetLocalPlayerUniqueId(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult);

	/** @return The local player's display name from the online identity interface. */
	UFUNCTION(Category = "Nexus|Utility", BlueprintCallable, meta = (WorldContext = "WorldContextObject", ExpandEnumAsExecs = "OutResult"))
	static FString GetLocalPlayerDisplayName(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult);

	/** Get the beacon manager from any world context. */
	UFUNCTION(Category = "Nexus|Beacons", BlueprintCallable, meta = (WorldContext = "WorldContextObject", ExpandEnumAsExecs = "OutResult"))
	static UNexusBeaconManager* GetBeaconManager(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult);

	UFUNCTION(Category = "Nexus|Reservation", BlueprintCallable, meta = (WorldContext = "WorldContextObject", ExpandEnumAsExecs = "OutResult"))
	static UNexusReservationManager* GetReservationManager(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult);

	UFUNCTION(Category = "Nexus|Party", BlueprintCallable, meta = (WorldContext = "WorldContextObject", ExpandEnumAsExecs = "OutResult"))
	static UNexusPartyManager* GetPartyManager(const UObject* WorldContextObject, ENexusBlueprintLibraryOutputResult& OutResult);
};
