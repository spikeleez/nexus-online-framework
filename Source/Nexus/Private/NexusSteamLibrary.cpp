// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusSteamLibrary.h"
#include "OnlineSubsystem.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "NexusLog.h"

#if NEXUS_WITH_STEAM
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END
#endif

UTexture2D* UNexusSteamLibrary::GetLocalPlayerSteamAvatar(ENexusBlueprintLibraryOutputResult& OutResult)
{
#if NEXUS_WITH_STEAM
	if (!IsSteamActive())
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	const CSteamID LocalSteamId = SteamUser()->GetSteamID();
	const int32 ImageHandle = SteamFriends()->GetLargeFriendAvatar(LocalSteamId);

	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return ConvertSteamImageToTexture(ImageHandle);
#else
	OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
	return nullptr;
#endif
}

UTexture2D* UNexusSteamLibrary::GetFriendSteamAvatar(const FUniqueNetIdRepl& FriendId, ENexusBlueprintLibraryOutputResult& OutResult)
{
#if NEXUS_WITH_STEAM
	if (!IsSteamActive() || !FriendId.IsValid())
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	// Convert the Unreal unique ID string to a Steam CSteamID.
	const FString IdString = FriendId.ToString();
	const uint64 SteamId64 = FCString::Atoi64(*IdString);

	if (SteamId64 == 0)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("Could not parse Steam ID from '%s'."), *IdString);
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return nullptr;
	}

	const CSteamID FriendSteamId(SteamId64);
	const int32 ImageHandle = SteamFriends()->GetLargeFriendAvatar(FriendSteamId);

	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return ConvertSteamImageToTexture(ImageHandle);
#else
	OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
	return nullptr;
#endif
}

FString UNexusSteamLibrary::GetLocalPlayerSteamName(ENexusBlueprintLibraryOutputResult& OutResult)
{
#if NEXUS_WITH_STEAM
	if (!IsSteamActive())
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return FString();
	}

	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return FString(UTF8_TO_TCHAR(SteamFriends()->GetPersonaName()));
#else
	OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
	return FString();
#endif
}

FString UNexusSteamLibrary::GetLocalPlayerSteamId(ENexusBlueprintLibraryOutputResult& OutResult)
{
#if NEXUS_WITH_STEAM
	if (!IsSteamActive())
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return FString();
	}

	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return FString::Printf(TEXT("%llu"), SteamUser()->GetSteamID().ConvertToUint64());
#else
	OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
	return FString();
#endif
}

FString UNexusSteamLibrary::GetFriendSteamName(const FUniqueNetIdRepl& FriendId, ENexusBlueprintLibraryOutputResult& OutResult)
{
#if NEXUS_WITH_STEAM
	if (!IsSteamActive() || !FriendId.IsValid())
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return FString();
	}

	const FString IdString = FriendId.ToString();
	const uint64 SteamId64 = FCString::Atoi64(*IdString);

	if (SteamId64 == 0)
	{
		OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
		return FString();
	}

	const CSteamID FriendSteamId(SteamId64);
	OutResult = ENexusBlueprintLibraryOutputResult::IsValid;
	return FString(UTF8_TO_TCHAR(SteamFriends()->GetFriendPersonaName(FriendSteamId)));
#else
	OutResult = ENexusBlueprintLibraryOutputResult::NotValid;
	return FString();
#endif
}

bool UNexusSteamLibrary::IsSteamActive()
{
#if NEXUS_WITH_STEAM
	const IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub)
	{
		return false;
	}

	return OnlineSub->GetSubsystemName() == STEAM_SUBSYSTEM && SteamAPI_Init != nullptr && SteamUser() != nullptr;
#else
	return false;
#endif
}

UTexture2D* UNexusSteamLibrary::ConvertSteamImageToTexture(int32 SteamImageHandle)
{
#if NEXUS_WITH_STEAM
	if (SteamImageHandle <= 0)
	{
		return nullptr;
	}

	// Get image dimensions from Steam.
	uint32 Width = 0;
	uint32 Height = 0;
	if (!SteamUtils()->GetImageSize(SteamImageHandle, &Width, &Height) || Width == 0 || Height == 0)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("Failed to get image size."));
		return nullptr;
	}

	// Allocate buffer for RGBA data.
	const int32 DataSize = Width * Height * 4;
	TArray<uint8> RawData;
	RawData.SetNumUninitialized(DataSize);

	if (!SteamUtils()->GetImageRGBA(SteamImageHandle, RawData.GetData(), DataSize))
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("Failed to get image RGBA data."));
		return nullptr;
	}

	// Create transient texture (not saved to disk).
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (!Texture)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to create texture."));
		return nullptr;
	}

	// Lock, copy, unlock.
	void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, RawData.GetData(), DataSize);
	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();

	// Push to GPU.
	Texture->UpdateResource();

	return Texture;
#else
	return nullptr;
#endif
}
