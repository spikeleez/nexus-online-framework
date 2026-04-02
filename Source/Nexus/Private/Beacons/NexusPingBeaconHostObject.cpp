#include "Beacons/NexusPingBeaconHostObject.h"
#include "Beacons/NexusPingBeaconClient.h"
#include "NexusOnlineSettings.h"

ANexusPingBeaconHostObject::ANexusPingBeaconHostObject()
{
	BeaconTypeName = ANexusPingBeaconClient::StaticClass()->GetName();
}

void ANexusPingBeaconHostObject::BeginPlay()
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
