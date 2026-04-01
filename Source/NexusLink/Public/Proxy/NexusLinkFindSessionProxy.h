// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusLinkTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusLinkFindSessionProxy.generated.h"

/** Delegate for the find sessions proxy completion, carrying the results array. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusLinkFindSessionsProxyComplete, const TArray<FNexusLinkSearchResult>&, Results);

/**
 * @class UNexusLinkFindSessionProxy 
 * 
 * Async Blueprint node for finding online sessions.
 * OnSuccess fires with results (may be empty if no sessions found but search succeeded).
 * OnFailure fires if the search itself failed.
 */
UCLASS()
class NEXUSLINK_API UNexusLinkFindSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the search completes. Results may be empty if none were found. */
	UPROPERTY(BlueprintAssignable)
	FNexusLinkFindSessionsProxyComplete OnSuccess;

	/** Fired when the search fails. */
	UPROPERTY(BlueprintAssignable)
	FNexusLinkFindSessionsProxyComplete OnFailure;

public:
	/**
	 * Find online sessions matching the given parameters.
	 *
	 * @param WorldContextObject World context.
	 * @param SearchParams Parameters for the search query.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session", DisplayName = "Find Nexus Sessions", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", Keywords = "Find"))
	static UNexusLinkFindSessionProxy* FindNexusLinkSessions(UObject* WorldContextObject, const FNexusLinkSearchParams& SearchParams);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:
	UFUNCTION()
	void OnFindComplete(ENexusLinkFindSessionsResult Result, const TArray<FNexusLinkSearchResult>& Results);

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FNexusLinkSearchParams SearchParams;
	FDelegateHandle NativeDelegateHandle;
};
