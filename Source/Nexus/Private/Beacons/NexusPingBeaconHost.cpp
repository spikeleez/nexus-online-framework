#include "Beacons/NexusPingBeaconHost.h"
#include "Beacons/NexusPingBeaconClient.h"
#include "NexusOnlineSettings.h"

ANexusPingBeaconHost::ANexusPingBeaconHost()
{
	BeaconTypeName = ANexusPingBeaconClient::StaticClass()->GetName();
}

void ANexusPingBeaconHost::BeginPlay()
{
	const UNexusOnlineSettings* NexusSettings = UNexusOnlineSettings::Get();
	if (NexusSettings && !NexusSettings->PingClientClass.IsNull())
	{
		ClientBeaconActorClass = NexusSettings->PingClientClass.LoadSynchronous();
	}
	else
	{
		ClientBeaconActorClass = ANexusPingBeaconClient::StaticClass();
	}
}
