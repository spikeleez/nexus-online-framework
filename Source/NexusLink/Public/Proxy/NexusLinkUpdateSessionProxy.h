#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusLinkTypes.h"
#include "NexusLinkUpdateSessionProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusLinkUpdateSessionResultDelegate, ENexusLinkUpdateSessionResult, Result);

UCLASS()
class NEXUSLINK_API UNexusLinkUpdateSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Pino de execução disparado quando a atualização é bem-sucedida. */
	UPROPERTY(BlueprintAssignable)
	FNexusLinkUpdateSessionResultDelegate OnSuccess;

	/** Pino de execução disparado em caso de falha (ex: spam de cliques, sem internet). */
	UPROPERTY(BlueprintAssignable)
	FNexusLinkUpdateSessionResultDelegate OnFailure;

public:
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session", DisplayName = "Update Nexus Session", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AdvancedDisplay = "SessionName", Keywords = "Update"))
	static UNexusLinkUpdateSessionProxy* UpdateNexusSession(UObject* WorldContextObject, FNexusLinkHostParams NewHostParams, FName SessionName);

	//~Begin UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~End of UOnlineBlueprintCallProxyBase interface

private:
	UFUNCTION()
	void OnSessionUpdatedCompleted(const ENexusLinkUpdateSessionResult Result);

private:
	UPROPERTY()
	UObject* WorldContextObject;

	FName SessionName;
	FNexusLinkHostParams HostParams;

	FDelegateHandle DelegateHandle;
};
