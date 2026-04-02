// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusJoinSessionProxy.generated.h"

/**
 * @class UNexusJoinSessionProxy
 * 
 * Async Blueprint node for joining an online session from a search result.
 */
UCLASS()
class NEXUS_API UNexusJoinSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the session is joined successfully. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FEmptyOnlineDelegate OnSuccess;

	/** Fired when joining the session fails. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FEmptyOnlineDelegate OnFailure;

public:
	/**
	 * Join an online session from a search result.
	 *
	 * @param WorldContextObject World context.
	 * @param SearchResult The session to join (from a previous FindSessions call).
	 * @param SessionName Session name. Defaults to the name configured in Plugin Settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Session", DisplayName = "Join Nexus Session", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", AdvancedDisplay = "SessionName", Keywords = "Join"))
	static UNexusJoinSessionProxy* JoinNexusSession(UObject* WorldContextObject, const FNexusSearchResult& SearchResult, FName SessionName = NAME_None, const bool bAutoTravel = true);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:
	UFUNCTION()
	void OnJoinComplete(ENexusJoinSessionResult Result);

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FNexusSearchResult SearchResult;
	FName SessionName;
	uint8 bAutoTravel:1;
	FDelegateHandle NativeDelegateHandle;
};
