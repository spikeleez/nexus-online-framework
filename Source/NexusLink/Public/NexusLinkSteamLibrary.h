// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Texture2D.h"
#include "NexusLinkTypes.h"
#include "NexusLinkSteamLibrary.generated.h"

/**
 * @class UNexusLinkSteamLibrary
 * 
 * Steam-specific helper functions for NexusLink.
 * All functions check at runtime whether Steam is the active OSS.
 * If Steam is not active, they return nullptr / empty values gracefully.
 */
UCLASS()
class NEXUSLINK_API UNexusLinkSteamLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/**
	 * Get the local player's Steam avatar as a texture.
	 *
	 * @return The avatar as UTexture2D, or nullptr if unavailable.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Steam|Avatar", meta = (ExpandEnumAsExecs = "OutResult"))
	static UTexture2D* GetLocalPlayerSteamAvatar(ENexusBlueprintLibraryOutputResult& OutResult);

	/**
	 * Get a friend's Steam avatar as a texture.
	 *
	 * @param FriendId The unique net ID of the friend.
	 * @return The avatar as UTexture2D, or nullptr if unavailable.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Steam|Avatar", meta = (ExpandEnumAsExecs = "OutResult"))
	static UTexture2D* GetFriendSteamAvatar(const FUniqueNetIdRepl& FriendId, ENexusBlueprintLibraryOutputResult& OutResult);

	/** @return The local player's Steam display name. Empty if Steam is not active. */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Steam|Identity", meta = (ExpandEnumAsExecs = "OutResult"))
	static FString GetLocalPlayerSteamName(ENexusBlueprintLibraryOutputResult& OutResult);

	/** @return The local player's Steam ID as a string (e.g. "76561198XXXXXXXXX"). */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Steam|Identity", meta = (ExpandEnumAsExecs = "OutResult"))
	static FString GetLocalPlayerSteamId(ENexusBlueprintLibraryOutputResult& OutResult);

	/** @return The Steam persona name of a friend. Empty if not found. */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Steam|Identity", meta = (ExpandEnumAsExecs = "OutResult"))
	static FString GetFriendSteamName(const FUniqueNetIdRepl& FriendId, ENexusBlueprintLibraryOutputResult& OutResult);

	/** @return Whether the active online subsystem is Steam. */
	UFUNCTION(BlueprintPure, Category = "NexusLink|Steam|Status")
	static bool IsSteamActive();

private:
	/**
	 * Internal: Convert a Steam image handle to a UTexture2D.
	 *
	 * @param SteamImageHandle The image handle from ISteamFriends avatar functions.
	 * @return The texture, or nullptr on failure.
	 */
	static UTexture2D* ConvertSteamImageToTexture(int32 SteamImageHandle);
};
