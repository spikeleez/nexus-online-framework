// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusFindSessionProxy.generated.h"

/** Delegate for the find sessions proxy completion, carrying the results array. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusFindSessionsProxyComplete, const TArray<FNexusSearchResult>&, Results);

/**
 * @class UNexusFindSessionProxy 
 * 
 * Async Blueprint node for finding online sessions.
 * OnSuccess fires with results (may be empty if no sessions found but search succeeded).
 * OnFailure fires if the search itself failed.
 */
UCLASS()
class NEXUS_API UNexusFindSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the search completes. Results may be empty if none were found. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusFindSessionsProxyComplete OnSuccess;

	/** Fired when the search fails. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusFindSessionsProxyComplete OnFailure;

public:
	/**
	 * Find online sessions matching the given parameters.
	 *
	 * @param WorldContextObject World context.
	 * @param SearchParams Parameters for the search query.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session", DisplayName = "Find Nexus Sessions", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", Keywords = "Find"))
	static UNexusFindSessionProxy* FindNexusSessions(UObject* WorldContextObject, const FNexusSearchParams& SearchParams);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:
	UFUNCTION()
	void OnFindComplete(ENexusFindSessionsResult Result, const TArray<FNexusSearchResult>& Results);

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FNexusSearchParams SearchParams;
	FDelegateHandle NativeDelegateHandle;
};
