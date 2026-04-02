// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Texture2D.h"
#include "NexusOnlineTypes.h"
#include "NexusSteamLibrary.generated.h"

/**
 * @class UNexusSteamLibrary
 * 
 * Steam-specific helper functions for Nexus.
 * All functions check at runtime whether Steam is the active OSS.
 * If Steam is not active, they return nullptr / empty values gracefully.
 */
UCLASS()
class NEXUS_API UNexusSteamLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** @return Whether the active online subsystem is Steam. */
	UFUNCTION(Category = "Nexus|Steam", BlueprintPure)
	static bool IsSteamActive();

	/** @return The avatar as UTexture2D, or nullptr if unavailable. */
	UFUNCTION(Category = "Nexus|Steam|Local", BlueprintCallable, meta = (ExpandEnumAsExecs = "OutResult"))
	static UTexture2D* GetLocalPlayerSteamAvatar(ENexusBlueprintLibraryOutputResult& OutResult);

	/** @return The local player's Steam display name. Empty if Steam is not active. */
	UFUNCTION(Category = "Nexus|Steam|Local", BlueprintCallable, meta = (ExpandEnumAsExecs = "OutResult"))
	static FString GetLocalPlayerSteamName(ENexusBlueprintLibraryOutputResult& OutResult);

	/** @return The local player's Steam ID as a string (e.g. "76561198XXXXXXXXX"). */
	UFUNCTION(Category = "Nexus|Steam|Local", BlueprintCallable, meta = (ExpandEnumAsExecs = "OutResult"))
	static FString GetLocalPlayerSteamId(ENexusBlueprintLibraryOutputResult& OutResult);

	/** @return The friend's steam avatar texture. */
	UFUNCTION(Category = "Nexus|Steam|Friend", BlueprintCallable, meta = (ExpandEnumAsExecs = "OutResult"))
	static UTexture2D* GetFriendSteamAvatar(const FUniqueNetIdRepl& FriendId, ENexusBlueprintLibraryOutputResult& OutResult);

	/** @return The Steam persona name of a friend. Empty if not found. */
	UFUNCTION(Category = "Nexus|Steam|Friend", BlueprintCallable, meta = (ExpandEnumAsExecs = "OutResult"))
	static FString GetFriendSteamName(const FUniqueNetIdRepl& FriendId, ENexusBlueprintLibraryOutputResult& OutResult);

private:
	/**
	 * Internal: Convert a Steam image handle to a UTexture2D.
	 *
	 * @param SteamImageHandle The image handle from ISteamFriends avatar functions.
	 * @return The texture, or nullptr on failure.
	 */
	static UTexture2D* ConvertSteamImageToTexture(int32 SteamImageHandle);
};
