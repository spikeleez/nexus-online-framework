// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusOnlineSubsystem.h"
#include "Managers/NexusSessionManager.h"
#include "Managers/NexusFriendManager.h"
#include "NexusOnlineSettings.h"
#include "OnlineSubsystem.h"
#include "Engine/GameInstance.h"
#include "NexusLog.h"
#include "Engine/World.h"
#include "Managers/NexusBeaconManager.h"
#include "Managers/NexusReservationManager.h"

UNexusOnlineSubsystem::UNexusOnlineSubsystem()
	: SessionManager(nullptr)
	, FriendManager(nullptr)
{

}

void UNexusOnlineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameInstance* GameInstance = GetGameInstance();
	check(GameInstance);

	// Create and initialize managers. They are owned by this subsystem (Outer = this).
	SessionManager = NewObject<UNexusSessionManager>(this);
	SessionManager->Initialize(GameInstance);

	FriendManager = NewObject<UNexusFriendManager>(this);
	FriendManager->Initialize(GameInstance);

	BeaconManager = NewObject<UNexusBeaconManager>(this);
	BeaconManager->Initialize(GameInstance);

	SessionManager->NativeOnSessionCreated.AddUObject(BeaconManager, &UNexusBeaconManager::OnSessionCreated);
	SessionManager->NativeOnSessionDestroyed.AddUObject(BeaconManager, &UNexusBeaconManager::OnSessionDestroyed);

	ReservationManager = NewObject<UNexusReservationManager>(this);
	ReservationManager->Initialize(GameInstance);

	SessionManager->NativeOnSessionCreated.AddUObject(ReservationManager, &UNexusReservationManager::OnSessionCreated);
	SessionManager->NativeOnSessionDestroyed.AddUObject(ReservationManager, &UNexusReservationManager::OnSessionDestroyed);

	NEXUS_LOG(LogNexus, Log, TEXT("Initialized. Online subsystem: %s"), *GetOnlineSubsystemName());
}

void UNexusOnlineSubsystem::Deinitialize()
{
	if (SessionManager)
	{
		SessionManager->Deinitialize();
		SessionManager = nullptr;
	}

	if (FriendManager)
	{
		FriendManager->Deinitialize();
		FriendManager = nullptr;
	}

	if (BeaconManager)
	{
		BeaconManager->Deinitialize();
		BeaconManager = nullptr;
	}

	if (ReservationManager)
	{
		ReservationManager->Deinitialize();
		ReservationManager = nullptr;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Deinitialized."));
	Super::Deinitialize();
}

bool UNexusOnlineSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Always create. Gate via settings in the future if needed.
	return true;
}

UNexusOnlineSubsystem* UNexusOnlineSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UNexusOnlineSubsystem>();
}

bool UNexusOnlineSubsystem::IsOnlineSubsystemAvailable()
{
	return IOnlineSubsystem::Get() != nullptr;
}

FString UNexusOnlineSubsystem::GetOnlineSubsystemName()
{
	const IOnlineSubsystem* OnlineSubystem = IOnlineSubsystem::Get();
	if (!OnlineSubystem)
	{
		return TEXT("None");
	}
	return OnlineSubystem->GetSubsystemName().ToString();
}
