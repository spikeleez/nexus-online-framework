#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "NexusBeaconManager.generated.h"

class AOnlineBeaconHost;
class ANexusPingBeaconHostObject;
class ANexusPingBeaconClient;
class UGameInstance;

/**
 * @class UNexusBeaconManager
 * 
 * Central manager for all Online Beacons in the NexusOnline plugin.
 * Owned by UNexusOnlineSubsystem.
 */
UCLASS()
class NEXUS_API UNexusBeaconManager : public UObject
{
	GENERATED_BODY()

public:
	UNexusBeaconManager(const FObjectInitializer& ObjectInitializer);

	//~Begin UObject interface
	virtual UWorld* GetWorld() const override;
	//~End of UObject interface

	void Initialize(UGameInstance* InGameInstance);
	void Deinitialize();

	/** Starts the AOnlineBeaconHost on the configured port and registers all HostObjects. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Beacons")
	bool StartBeaconHost();

	/**
	 * Destroys all HostObject actors and the BeaconHost itself.
	 * Called automatically when the game session is destroyed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Beacons")
	void StopBeaconHost();

	/** @return Whether the BeaconHost is currently listening for connections. */
	UFUNCTION(BlueprintPure, Category = "Nexus|Beacons")
	bool IsBeaconHostActive() const { return bBeaconHostActive; }

	/** Spawns a ping beacon and connects it via a raw address string. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Beacons")
	ANexusPingBeaconClient* ConnectPingBeacon(const FString& HostAddress);

	/** Spawns a ping beacon and connects it using an OSS session search result. */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Beacons")
	ANexusPingBeaconClient* ConnectPingBeaconToSession(const FNexusSearchResult& SearchResult);

	/** Bound to SessionManager::NativeOnSessionCreated. Starts the host automatically. */
	virtual void OnSessionCreated(ENexusCreateSessionResult Result);

	/** Bound to SessionManager::NativeOnSessionDestroyed. Stops the host automatically. */
	virtual void OnSessionDestroyed(ENexusDestroySessionResult Result);

private:
	/** Spawns and registers all configured HostObjects. Add new types here. */
	bool RegisterAllHostObjects();

	/** Spawns and registers the ping HostObject. */
	bool RegisterPingHostObject();

	/** Resolves PingClientClass from settings with fallback to ANexusPingBeaconClient. */
	TSubclassOf<ANexusPingBeaconClient> ResolvePingClientClass() const;

	/** Resolves PingHostObjectClass from settings with fallback to ANexusPingBeaconHostObject. */
	TSubclassOf<ANexusPingBeaconHostObject> ResolvePingHostObjectClass() const;

private:
	/** The single beacon host. All HostObjects are registered here. */
	UPROPERTY()
	TObjectPtr<AOnlineBeaconHost> BeaconHost;

	/** Handles incoming ping beacon connections. */
	UPROPERTY()
	TObjectPtr<ANexusPingBeaconHostObject> PingHostObject;

	TWeakObjectPtr<UGameInstance> CachedGameInstance;

	uint8 bBeaconHostActive:1;
};
