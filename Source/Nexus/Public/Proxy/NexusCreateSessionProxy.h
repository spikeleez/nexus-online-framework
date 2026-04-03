// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusCreateSessionProxy.generated.h"

/**
 * @class UNexusCreateSessionProxy
 * 
 * Async Blueprint node for creating an online session.
 * Provides OnSuccess and OnFailure execution pins in the Blueprint graph.
 */
UCLASS()
class NEXUS_API UNexusCreateSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the session is created successfully. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FEmptyOnlineDelegate OnSuccess;

	/** Fired when session creation fails for any reason. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FEmptyOnlineDelegate OnFailure;

public:
	/**
	 * Create a new online session with the given parameters.
	 *
	 * @param WorldContextObject World context.
	 * @param HostParams Parameters defining the session properties.
	 * @param SessionName Session name. Defaults to the name configured in Plugin Settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session", DisplayName = "Create Nexus Session", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", AdvancedDisplay = "SessionName", Keywords = "Create"))
	static UNexusCreateSessionProxy* CreateNexusSession(UObject* WorldContextObject, const FNexuHostParams& HostParams, FName SessionName = NAME_None);

	//~ Begin UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~ End UOnlineBlueprintCallProxyBase interface

private:
	UFUNCTION()
	void OnCreateComplete(ENexusCreateSessionResult Result);

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FNexuHostParams HostParams;
	FName SessionName;
	FDelegateHandle NativeDelegateHandle;
};
