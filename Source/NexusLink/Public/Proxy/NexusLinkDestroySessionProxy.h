// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusLinkTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusLinkDestroySessionProxy.generated.h"

/**
 * @class UNexusLinkDestroySessionProxy
 * 
 * Async Blueprint node for destroying an active online session.
 */
UCLASS()
class NEXUSLINK_API UNexusLinkDestroySessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the session is destroyed successfully. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	/** Fired when session destruction fails. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

public:
	/**
	 * Destroy the current session.
	 *
	 * @param WorldContextObject World context.
	 * @param SessionName Session name to destroy. Defaults to the name configured in Plugin Settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session", DisplayName = "Destroy Nexus Session", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", AdvancedDisplay = "SessionName", Keywords = "Destroy"))
	static UNexusLinkDestroySessionProxy* DestroyNexusLinkSession(UObject* WorldContextObject, FName SessionName = NAME_None);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~ End UOnlineBlueprintCallProxyBase Interface


private:
	UFUNCTION()
	void OnDestroyComplete(ENexusLinkDestroySessionResult Result);

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FName SessionName;
	FDelegateHandle NativeDelegateHandle;
};
