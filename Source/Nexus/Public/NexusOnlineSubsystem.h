// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NexusOnlineTypes.h"
#include "NexusOnlineSubsystem.generated.h"

class UNexusSessionManager;
class UNexusFriendManager;
class UNexusBeaconManager;
class UNexusReservationManager;

/**
 * @class UNexusOnlineSubsystem
 * 
 * Core subsystem for the Nexus plugin.
 * Automatically created when the GameInstance initializes.
 * Owns and provides access to the session and friend managers.
 */
UCLASS(NotBlueprintType, NotBlueprintable)
class NEXUS_API UNexusOnlineSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UNexusOnlineSubsystem();

	//~Begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~End of USubsystem interface
	
	/** @return The session manager responsible for create/find/join/destroy operations. */
	UNexusSessionManager* GetSessionManager() const { return SessionManager; }

	/** @return The friend manager responsible for friends list and invites. */
	UNexusFriendManager* GetFriendManager() const { return FriendManager; }

	/** @return The beacon manager responsible for all Online Beacon lifecycle. */
	UNexusBeaconManager* GetBeaconManager() const { return BeaconManager; }

	/** @return The beacon manager responsible for all Online Beacon lifecycle. */
	UNexusReservationManager* GetReservationManager() const { return ReservationManager; }

	/**
	 * Get the Nexus subsystem from any world context object.
	 * This is the recommended way to access Nexus from anywhere.
	 */
	static UNexusOnlineSubsystem* Get(const UObject* WorldContextObject);

	/** @return Whether any online subsystem is currently available. */
	static bool IsOnlineSubsystemAvailable();

	/** @return The name of the currently active online subsystem (e.g. "Steam", "EOS", "NULL"). */
	static FString GetOnlineSubsystemName();

protected:
	/** Session management instance. Created during Initialize. */
	UPROPERTY()
	TObjectPtr<UNexusSessionManager> SessionManager;

	/** Friend management instance. Created during Initialize. */
	UPROPERTY()
	TObjectPtr<UNexusFriendManager> FriendManager;

	/** Beacon management instance. Created during Initialize. */
	UPROPERTY()
	TObjectPtr<UNexusBeaconManager> BeaconManager;

	/** Reservation management instance. Created during Initialize. */
	UPROPERTY()
	TObjectPtr<UNexusReservationManager> ReservationManager;
};
