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
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	/** Fired when joining the session fails. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

public:
	/**
	 * Join an online session from a search result.
	 *
	 * @param WorldContextObject World context.
	 * @param SearchResult The session to join (from a previous FindSessions call).
	 * @param SessionName Session name. Defaults to the name configured in Plugin Settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session", DisplayName = "Join Nexus Session", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", AdvancedDisplay = "SessionName"))
	static UNexusLinkJoinSessionProxy* JoinNexusLinkSession(UObject* WorldContextObject, const FNexusLinkSearchResult& SearchResult, FName SessionName = NAME_None);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

	virtual void BeginDestroy() override;

private:
	void OnJoinComplete(ENexusLinkJoinSessionResult Result);
	void Cleanup();

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FNexusLinkSearchResult SearchResult;
	FName SessionName;
	FDelegateHandle NativeDelegateHandle;
};
