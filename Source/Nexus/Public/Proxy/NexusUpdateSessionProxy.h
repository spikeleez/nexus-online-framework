#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusOnlineTypes.h"
#include "NexusUpdateSessionProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusUpdateSessionProxyComplete, ENexusUpdateSessionResult, Result);

/**
 * @class UNexusJoinSessionProxy
 *
 * Async Blueprint node for updating an online session.
 */
UCLASS()
class NEXUS_API UNexusUpdateSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the update session completes. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusUpdateSessionProxyComplete OnSuccess;

	/** Fired when the update session failed. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusUpdateSessionProxyComplete OnFailure;

public:
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session", DisplayName = "Update Nexus Session", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AdvancedDisplay = "SessionName", Keywords = "Update"))
	static UNexusUpdateSessionProxy* UpdateNexusSession(UObject* WorldContextObject, FNexusHostParams NewHostParams, FName SessionName);

	//~Begin UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~End of UOnlineBlueprintCallProxyBase interface

private:
	UFUNCTION()
	void OnSessionUpdatedCompleted(const ENexusUpdateSessionResult Result);

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FName SessionName;
	FNexusHostParams HostParams;

	FDelegateHandle DelegateHandle;
};
