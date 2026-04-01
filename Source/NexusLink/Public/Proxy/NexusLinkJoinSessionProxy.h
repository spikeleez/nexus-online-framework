// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusLinkTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusLinkJoinSessionProxy.generated.h"

/**
 * @class UNexusLinkJoinSessionProxy
 * 
 * Async Blueprint node for joining an online session from a search result.
 */
UCLASS()
class NEXUSLINK_API UNexusLinkJoinSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the session is joined successfully. */
	UPROPERTY(BlueprintAssignable, Category = "NexusLink|Events")
	FEmptyOnlineDelegate OnSuccess;

	/** Fired when joining the session fails. */
	UPROPERTY(BlueprintAssignable, Category = "NexusLink|Events")
	FEmptyOnlineDelegate OnFailure;

public:
	/**
	 * Join an online session from a search result.
	 *
	 * @param WorldContextObject World context.
	 * @param SearchResult The session to join (from a previous FindSessions call).
	 * @param SessionName Session name. Defaults to the name configured in Plugin Settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session", DisplayName = "Join Nexus Session", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", AdvancedDisplay = "SessionName", Keywords = "Join"))
	static UNexusLinkJoinSessionProxy* JoinNexusSession(UObject* WorldContextObject, const FNexusLinkSearchResult& SearchResult, FName SessionName = NAME_None, const bool bAutoTravel = true);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

private:
	UFUNCTION()
	void OnJoinComplete(ENexusLinkJoinSessionResult Result);

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FNexusLinkSearchResult SearchResult;
	FName SessionName;
	uint8 bAutoTravel:1;
	FDelegateHandle NativeDelegateHandle;
};
