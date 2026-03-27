// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NexusLinkTypes.h"
#include "NexusLinkBlueprintLibrary.generated.h"

class UNexusLinkSubsystem;
class UNexusLinkSessionManager;
class UNexusLinkFriendManager;

/**
 * @class UNexusLinkBlueprintLibrary
 * 
 * Static Blueprint function library for NexusLink.
 * Provides convenience accessors, factory functions, and utility helpers
 * that can be called from any Blueprint without needing a direct reference
 * to the subsystem or its managers.
 */
UCLASS()
class NEXUSLINK_API UNexusLinkBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** Get the NexusLink subsystem from any world context. */
	UFUNCTION(BlueprintPure, Category = "NexusLink", meta = (WorldContext = "WorldContextObject"))
	static UNexusLinkSubsystem* GetNexusLinkSubsystem(const UObject* WorldContextObject);

	/** Get the session manager from any world context. */
	UFUNCTION(BlueprintPure, Category = "NexusLink", meta = (WorldContext = "WorldContextObject"))
	static UNexusLinkSessionManager* GetNexusLinkSessionManager(const UObject* WorldContextObject);

	/** Get the friend manager from any world context. */
	UFUNCTION(BlueprintPure, Category = "NexusLink", meta = (WorldContext = "WorldContextObject"))
	static UNexusLinkFriendManager* GetNexusLinkFriendManager(const UObject* WorldContextObject);

	/** @return Whether any online subsystem is available. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Utility")
	static bool IsOnlineSubsystemAvailable();

	/** @return Name of the active online subsystem (e.g. "Steam", "EOS", "NULL"). */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Utility")
	static FString GetActiveOnlineSubsystemName();

	/** @return The local player's unique net ID from the online subsystem. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Utility", meta = (WorldContext = "WorldContextObject"))
	static FUniqueNetIdRepl GetLocalPlayerUniqueId(const UObject* WorldContextObject);

	/** @return The local player's display name from the online identity interface. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Utility", meta = (WorldContext = "WorldContextObject"))
	static FString GetLocalPlayerDisplayName(const UObject* WorldContextObject);

	/** Create a FNexusLinkHostParams populated with defaults from Plugin Settings. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session")
	static FNexusLinkHostParams MakeDefaultHostParams();

	/** Create a FNexusLinkSearchParams populated with defaults from Plugin Settings. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session")
	static FNexusLinkSearchParams MakeDefaultSearchParams();

	/** Create a session setting from an int32 value. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session|Settings", DisplayName = "Make Session Setting (int32)", meta = (NativeMakeFunc))
	static FNexusLinkSessionSetting MakeSessionSettingInt32(FName Key, int32 Value, bool bAdvertise = true);

	/** Create a session setting from a string value. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session|Settings", DisplayName = "Make Session Setting (String)", meta = (NativeMakeFunc))
	static FNexusLinkSessionSetting MakeSessionSettingString(FName Key, FString Value, bool bAdvertise = true);

	/** Create a session setting from a float value. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session|Settings", DisplayName = "Make Session Setting (float)", meta = (NativeMakeFunc))
	static FNexusLinkSessionSetting MakeSessionSettingFloat(FName Key, float Value, bool bAdvertise = true);

	/** Create a query setting from an int32 value. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session|Settings", DisplayName = "Make Query Setting (int32)", meta = (NativeMakeFunc))
	static FNexusLinkQuerySetting MakeQuerySettingInt32(FName Key, int32 Value, ENexusLinkQueryComparisonOp Comparison);

	/** Create a query setting from a string value. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session|Settings", DisplayName = "Make Query Setting (String)", meta = (NativeMakeFunc))
	static FNexusLinkQuerySetting MakeQuerySettingString(FName Key, FString Value, ENexusLinkQueryComparisonOp Comparison);

	/** Create a query setting from a float value. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session|Settings", DisplayName = "Make Query Setting (float)", meta = (NativeMakeFunc))
	static FNexusLinkQuerySetting MakeQuerySettingFloat(FName Key, float Value, ENexusLinkQueryComparisonOp Comparison);

	/** @return Whether the host params are valid. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsHostParamsValid(const FNexusLinkHostParams& HostParams);

	/** @return Whether the search params are valid. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsSearchParamsValid(const FNexusLinkSearchParams& SearchParams);

	/** @return Whether the search result is valid. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsSearchResultValid(const FNexusLinkSearchResult& SearchResult);

	/** @return Whether the named session is valid. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsNamedSessionValid(const FNexusLinkNamedSession& NamedSession);

	/** @return Whether the online friend is valid. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Friends", DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsOnlineFriendValid(const FNexusLinkOnlineFriend& Friend);

	/** @return Whether the pending invite is valid. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Friends", DisplayName = "Is Valid", meta = (ScriptMethod = "IsValid"))
	static bool IsPendingInviteValid(const FNexusLinkPendingInvite& Invite);

	/** @return The session settings from a search result. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static FNexusLinkSessionSettings GetSearchResultSettings(const FNexusLinkSearchResult& SearchResult);

	/** @return The owner's unique ID from a search result. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static FUniqueNetIdRepl GetSearchResultOwnerId(const FNexusLinkSearchResult& SearchResult);

	/** @return The owner's display name from a search result. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static FString GetSearchResultOwnerName(const FNexusLinkSearchResult& SearchResult);

	/** @return Current player count from a search result. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static int32 GetSearchResultPlayerCount(const FNexusLinkSearchResult& SearchResult);

	/** @return Max player count from a search result. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static int32 GetSearchResultMaxPlayers(const FNexusLinkSearchResult& SearchResult);

	/** @return Open slots from a search result. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static int32 GetSearchResultOpenSlots(const FNexusLinkSearchResult& SearchResult);

	/** @return Ping from a search result. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static int32 GetSearchResultPing(const FNexusLinkSearchResult& SearchResult);

	/** @return The session name from a named session. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static FName GetNamedSessionName(const FNexusLinkNamedSession& NamedSession);

	/** @return The session state from a named session. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static ENexusLinkSessionState GetNamedSessionState(const FNexusLinkNamedSession& NamedSession);

	/** @return The session settings from a named session. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static FNexusLinkSessionSettings GetNamedSessionSettings(const FNexusLinkNamedSession& NamedSession);

	/** @return Player count from a named session. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session", meta = (ScriptMethod))
	static int32 GetNamedSessionPlayerCount(const FNexusLinkNamedSession& NamedSession);

	/** Get a custom int32 setting from session settings. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session|Settings", DisplayName = "Get Custom Setting (int32)", meta = (ReturnDisplayName = "Success", AutoCreateRefTerm = "Value"))
	static bool GetCustomSettingInt32(const FNexusLinkSessionSettings& Settings, FName Key, int32& Value);

	/** Get a custom string setting from session settings. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session|Settings", DisplayName = "Get Custom Setting (String)", meta = (ReturnDisplayName = "Success", AutoCreateRefTerm = "Value"))
	static bool GetCustomSettingString(const FNexusLinkSessionSettings& Settings, FName Key, FString& Value);

	/** Get a custom float setting from session settings. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Session|Settings", DisplayName = "Get Custom Setting (float)", meta = (ReturnDisplayName = "Success", AutoCreateRefTerm = "Value"))
	static bool GetCustomSettingFloat(const FNexusLinkSessionSettings& Settings, FName Key, float& Value);

	UFUNCTION(BlueprintCallable, Category = "NexusLink|Travel", meta = (WorldContext = "WorldContextObject"))
	static void ClientTravelToSession(const UObject* WorldContextObject, const FString& ConnectString);
};
