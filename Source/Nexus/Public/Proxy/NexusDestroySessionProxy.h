// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusDestroySessionProxy.generated.h"

/**
 * @class UNexusDestroySessionProxy
 * 
 * Async Blueprint node for destroying an active online session.
 */
UCLASS()
class NEXUS_API UNexusDestroySessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the session is destroyed successfully. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FEmptyOnlineDelegate OnSuccess;

	/** Fired when session destruction fails. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FEmptyOnlineDelegate OnFailure;

public:
	/**
	 * Destroy the current session.
	 *
	 * @param WorldContextObject World context.
	 * @param SessionName Session name to destroy. Defaults to the name configured in Plugin Settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session", DisplayName = "Destroy Nexus Session", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", AdvancedDisplay = "SessionName", Keywords = "Destroy"))
	static UNexusDestroySessionProxy* DestroyNexusSession(UObject* WorldContextObject, FName SessionName = NAME_None);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~ End UOnlineBlueprintCallProxyBase Interface


private:
	UFUNCTION()
	void OnDestroyComplete(ENexusDestroySessionResult Result);

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FName SessionName;
	FDelegateHandle NativeDelegateHandle;
};
