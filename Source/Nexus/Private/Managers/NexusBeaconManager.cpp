#include "Managers/NexusBeaconManager.h"
#include "Beacons/NexusPingBeaconClient.h"
#include "Beacons/NexusPingBeaconHost.h"
#include "NexusOnlineSettings.h"
#include "NexusOnlineTypes.h"
#include "NexusLog.h"
#include "OnlineBeaconHost.h"
#include "Beacons/NexusPartyBeaconHost.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UNexusBeaconManager::UNexusBeaconManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, BeaconHost(nullptr)
	, PingHost(nullptr)
	, bBeaconHostActive(false)
{

}

UWorld* UNexusBeaconManager::GetWorld() const
{
	return CachedGameInstance.IsValid() ? CachedGameInstance->GetWorld() : nullptr;
}

void UNexusBeaconManager::Initialize(UGameInstance* InGameInstance)
{
	check(InGameInstance);
	CachedGameInstance = InGameInstance;
	NEXUS_LOG(LogNexus, Log, TEXT("BeaconManager initialized."));
}

void UNexusBeaconManager::Deinitialize()
{
	StopBeaconHost();
	CachedGameInstance.Reset();
	NEXUS_LOG(LogNexus, Log, TEXT("BeaconManager deinitialized."));
}

bool UNexusBeaconManager::StartBeaconHost()
{
	if (bBeaconHostActive)
	{
		NEXUS_LOG(LogNexus, Verbose, TEXT("BeaconHost already active - skipping."));
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("World is null."));
		return false;
	}

	// Beacon hosting is only valid on servers and listen-servers.
	const ENetMode NetMode = World->GetNetMode();
	if (NetMode == NM_Client)
	{
		NEXUS_LOG(LogNexus, Log, TEXT("Skipped - not running on a server."));
		return false;
	}

	const UNexusOnlineSettings* NexusSettings = UNexusOnlineSettings::Get();

	BeaconHost = World->SpawnActor<AOnlineBeaconHost>();
	if (!BeaconHost)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to spawn AOnlineBeaconHost."));
		return false;
	}

	BeaconHost->ListenPort = NexusSettings->BeaconListenPort;

	if (!BeaconHost->InitHost())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("InitHost() failed on port %d. Port may already be in use."), NexusSettings->BeaconListenPort);
		BeaconHost->Destroy();
		BeaconHost = nullptr;
		return false;
	}

	if (!RegisterAllHostObjects())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to register HostObjects."));
		BeaconHost->Destroy();
		BeaconHost = nullptr;
		return false;
	}

	// Unpausing begins accepting incoming connections.
	BeaconHost->PauseBeaconRequests(false);

	bBeaconHostActive = true;
	NEXUS_LOG(LogNexus, Log, TEXT("BeaconHost started on port %d"), NexusSettings->BeaconListenPort);
	return true;
}

void UNexusBeaconManager::StopBeaconHost()
{
	if (!bBeaconHostActive)
	{
		return;
	}

	if (IsValid(PingHost))
	{
		PingHost->Destroy();
		PingHost = nullptr;
	}

	if (IsValid(BeaconHost))
	{
		BeaconHost->Destroy();
		BeaconHost = nullptr;
	}
	
	if (IsValid(PartyHost))
	{
		// DisbandParty is handled by UNexusPartyManager::Deinitialize before this is called.
		// Destroy the actor unconditionally.
		PartyHost->Destroy();
		PartyHost = nullptr;
	}

	bBeaconHostActive = false;
	NEXUS_LOG(LogNexus, Log, TEXT("BeaconHost stopped."));
}

ANexusPingBeaconClient* UNexusBeaconManager::ConnectPingBeacon(const FString& HostAddress)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("World is null."));
		return nullptr;
	}

	if (HostAddress.IsEmpty())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("HostAddress is empty."));
		return nullptr;
	}

	const TSubclassOf<ANexusPingBeaconClient> ClientClass = ResolvePingClientClass();
	ANexusPingBeaconClient* Client = World->SpawnActor<ANexusPingBeaconClient>(ClientClass);
	if (!Client)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to spawn client ping beacon actor."));
		return nullptr;
	}

	if (!Client->ConnectDirect(HostAddress))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("ConnectDirect failed for '%s'."), *HostAddress);
		Client->Destroy();
		return nullptr;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Initiated connection to '%s'."), *HostAddress);
	return Client;
}

ANexusPingBeaconClient* UNexusBeaconManager::ConnectPingBeaconToSession(const FNexusSearchResult& SearchResult)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("World is null."));
		return nullptr;
	}

	if (!SearchResult.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Search result is invalid."));
		return nullptr;
	}

	const TSubclassOf<ANexusPingBeaconClient> ClientClass = ResolvePingClientClass();
	ANexusPingBeaconClient* Client = World->SpawnActor<ANexusPingBeaconClient>(ClientClass);
	if (!Client)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to spawn client actor."));
		return nullptr;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Initiated connection via OSS."));
	return Client;
}

void UNexusBeaconManager::OnSessionCreated(ENexusCreateSessionResult Result)
{
	if (Result != ENexusCreateSessionResult::Success)
	{
		return;
	}

	if (UNexusOnlineSettings::Get()->bAutoStartBeaconHost)
	{
		StartBeaconHost();
	}
}

void UNexusBeaconManager::OnSessionDestroyed(ENexusDestroySessionResult Result)
{
	// Stop regardless of result? if the session is gone, beacons are irrelevant.
	StopBeaconHost();
}

bool UNexusBeaconManager::RegisterAllHostObjects()
{
	// Add future host object registrations here in order:
	// return RegisterPingHostObject() && RegisterLobbyHostObject() && ...
	return RegisterPingHostObject() && RegisterPartyHostObject();
}

bool UNexusBeaconManager::RegisterPingHostObject()
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(BeaconHost))
	{
		return false;
	}

	const TSubclassOf<ANexusPingBeaconHost> HostObjClass = ResolvePingHostObjectClass();
	PingHost = World->SpawnActor<ANexusPingBeaconHost>(HostObjClass);
	if (!PingHost)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to spawn HostObject."));
		return false;
	}

	const TSubclassOf<ANexusPingBeaconClient> ClientClass = ResolvePingClientClass();
	BeaconHost->RegisterHost(PingHost);

	NEXUS_LOG(LogNexus, Log, TEXT("Registered (BeaconType='%s')."), *PingHost->GetBeaconType());
	return true;
}

bool UNexusBeaconManager::RegisterPartyHostObject()
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(BeaconHost))
	{
		return false;
	}

	PartyHost = World->SpawnActor<ANexusPartyBeaconHost>();
	if (!IsValid(PartyHost))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to spawn ANexusPartyBeaconHost."));
		return false;
	}

	BeaconHost->RegisterHost(PartyHost);

	NEXUS_LOG(LogNexus, Log, TEXT("Registered ANexusPartyBeaconHost (BeaconType='%s')."), *PartyHost->GetBeaconType());
	return true;
}

TSubclassOf<ANexusPingBeaconClient> UNexusBeaconManager::ResolvePingClientClass() const
{
	const UNexusOnlineSettings* Settings = UNexusOnlineSettings::Get();
	if (!Settings->PingClientClass.IsNull())
	{
		if (UClass* Loaded = Settings->PingClientClass.LoadSynchronous())
		{
			if (Loaded->IsChildOf(ANexusPingBeaconClient::StaticClass()))
			{
				return Loaded;
			}
			NEXUS_LOG(LogNexus, Warning, TEXT("PingClientClass is not a child of ANexusPingBeaconClient. Using default."));
		}
	}
	return ANexusPingBeaconClient::StaticClass();
}

TSubclassOf<ANexusPingBeaconHost> UNexusBeaconManager::ResolvePingHostObjectClass() const
{
	const UNexusOnlineSettings* Settings = UNexusOnlineSettings::Get();
	if (!Settings->PingHostClass.IsNull())
	{
		if (UClass* Loaded = Settings->PingHostClass.LoadSynchronous())
		{
			if (Loaded->IsChildOf(ANexusPingBeaconHost::StaticClass()))
			{
				return Loaded;
			}
			NEXUS_LOG(LogNexus, Warning, TEXT("PingHostObjectClass is not a child of ANexusPingBeaconHostObject. Using default."));
		}
	}
	return ANexusPingBeaconHost::StaticClass();
}
