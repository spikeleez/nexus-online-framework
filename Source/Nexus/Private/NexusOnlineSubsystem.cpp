// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusOnlineSubsystem.h"
#include "Managers/NexusSessionManager.h"
#include "Managers/NexusFriendManager.h"
#include "OnlineSubsystem.h"
#include "Engine/GameInstance.h"
#include "NexusLog.h"
#include "NexusOnlineSettings.h"
#include "Engine/World.h"
#include "Managers/NexusBeaconManager.h"
#include "Managers/NexusPartyManager.h"
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
	
	const UNexusOnlineSettings* OnlineSettings = UNexusOnlineSettings::Get();
	if (IsValid(OnlineSettings))
	{
		// Create and initialize managers. They are owned by this subsystem (Outer = this).
		SessionManager = NewObject<UNexusSessionManager>(this, !OnlineSettings->SessionManagerClass.IsNull() ? OnlineSettings->SessionManagerClass.LoadSynchronous() : UNexusSessionManager::StaticClass());
		SessionManager->Initialize(GameInstance);

		FriendManager = NewObject<UNexusFriendManager>(this, !OnlineSettings->FriendManagerClass.IsNull() ? OnlineSettings->FriendManagerClass.LoadSynchronous() : UNexusFriendManager::StaticClass());
		FriendManager->Initialize(GameInstance);

		BeaconManager = NewObject<UNexusBeaconManager>(this, !OnlineSettings->BeaconManagerClass.IsNull() ? OnlineSettings->BeaconManagerClass.LoadSynchronous() : UNexusBeaconManager::StaticClass());
		BeaconManager->Initialize(GameInstance);

		SessionManager->NativeOnSessionCreated.AddUObject(BeaconManager, &UNexusBeaconManager::OnSessionCreated);
		SessionManager->NativeOnSessionDestroyed.AddUObject(BeaconManager, &UNexusBeaconManager::OnSessionDestroyed);
	
		PartyManager = NewObject<UNexusPartyManager>(this, !OnlineSettings->PartyManagerClass.IsNull() ? OnlineSettings->PartyManagerClass.LoadSynchronous() : UNexusPartyManager::StaticClass());
		PartyManager->Initialize(GameInstance, BeaconManager);

		ReservationManager = NewObject<UNexusReservationManager>(this, !OnlineSettings->ReservationManagerClass.IsNull() ? OnlineSettings->ReservationManagerClass.LoadSynchronous() : UNexusReservationManager::StaticClass());
		ReservationManager->Initialize(GameInstance);

		SessionManager->NativeOnSessionCreated.AddUObject(ReservationManager, &UNexusReservationManager::OnSessionCreated);
		SessionManager->NativeOnSessionDestroyed.AddUObject(ReservationManager, &UNexusReservationManager::OnSessionDestroyed);
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Initialized. Online subsystem: %s"), *GetOnlineSubsystemName());
}

void UNexusOnlineSubsystem::Deinitialize()
{
	if (IsValid(SessionManager))
	{
		SessionManager->Deinitialize();
		SessionManager = nullptr;
	}

	if (IsValid(FriendManager))
	{
		FriendManager->Deinitialize();
		FriendManager = nullptr;
	}
	
	if (IsValid(PartyManager))
	{
		PartyManager->Deinitialize();
		PartyManager = nullptr;
	}

	if (IsValid(ReservationManager))
	{
		ReservationManager->Deinitialize();
		ReservationManager = nullptr;
	}

	if (IsValid(BeaconManager))
	{
		BeaconManager->Deinitialize();
		BeaconManager = nullptr;
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
