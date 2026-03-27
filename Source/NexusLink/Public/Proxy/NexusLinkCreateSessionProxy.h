#pragma once

#include "CoreMinimal.h"
#include "NexusLinkTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusLinkCreateSessionProxy.generated.h"

/**
 * @class UNexusLinkCreateSessionProxy
 * 
 * Async Blueprint node for creating an online session.
 * Provides OnSuccess and OnFailure execution pins in the Blueprint graph.
 *
 * Usage: Right-click in Blueprint -> NexusLink|Session -> "Create NexusLink Session"
 */
UCLASS()
class NEXUSLINK_API UNexusLinkCreateSessionProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the session is created successfully. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	/** Fired when session creation fails for any reason. */
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

public:
	/**
	 * Create a new online session with the given parameters.
	 *
	 * @param WorldContextObject World context.
	 * @param HostParams Parameters defining the session properties.
	 * @param SessionName Session name. Defaults to the name configured in Plugin Settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "NexusLink|Session", DisplayName = "Create Nexus Session", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", AdvancedDisplay = "SessionName"))
	static UNexusLinkCreateSessionProxy* CreateNexusLinkSession(UObject* WorldContextObject, const FNexusLinkHostParams& HostParams, FName SessionName = NAME_None);

	//~ Begin UOnlineBlueprintCallProxyBase Interface
	virtual void Activate() override;
	//~ End UOnlineBlueprintCallProxyBase Interface

	virtual void BeginDestroy() override;

private:
	void OnCreateComplete(ENexusLinkCreateSessionResult Result);
	void Cleanup();

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FNexusLinkHostParams HostParams;
	FName SessionName;
	FDelegateHandle NativeDelegateHandle;
};
