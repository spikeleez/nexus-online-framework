#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "NexusPingBeaconHost.generated.h"

UCLASS(Blueprintable)
class NEXUS_API ANexusPingBeaconHost : public AOnlineBeaconHostObject
{
	GENERATED_BODY()

public:
	ANexusPingBeaconHost();

	//~Begin AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface
};
