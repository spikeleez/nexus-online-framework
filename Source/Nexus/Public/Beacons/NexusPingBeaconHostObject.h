#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "NexusPingBeaconHostObject.generated.h"

UCLASS(Blueprintable)
class NEXUS_API ANexusPingBeaconHostObject : public AOnlineBeaconHostObject
{
	GENERATED_BODY()

public:
	ANexusPingBeaconHostObject();

	//~Begin AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface
};
