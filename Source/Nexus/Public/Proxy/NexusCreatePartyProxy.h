// Copyright Spike Plugins 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusCreatePartyProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNexusCreatePartyProxyComplete, ENexusPartyResult, PartyResult, const FNexusPartyState&, PartyState);

/**
 * @class UNexusCreatePartyProxy
 *
 * Async Blueprint node for creating a party as the local player (leader).
 *
 * If Params.bCreateLobbySession is true (default), this node waits for two things:
 *   1. The party beacon to initialize (synchronous)
 *   2. The hidden lobby session to be created (async)
 * Only then does OnSuccess fire.
 *
 * Use Make Default Party Params to build the Params struct with sensible defaults.
 * The OnSuccess pin fires with the resulting PartyState ready for display in your UI.
 */
UCLASS()
class NEXUS_API UNexusCreatePartyProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()
	
public:
	/** Fired when the party (and lobby session, if requested) is created successfully. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusCreatePartyProxyComplete OnSuccess;
	
	/** Fired when party creation fails for any reason. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusCreatePartyProxyComplete OnFailure;
	
public:
	/**
	 * Create a party. The local player becomes the party leader.
	 *
	 * @param WorldContextObject World context.
	 * @param Params Party configuration. Use Make Default Party Params for sensible defaults.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party", DisplayName = "Create Nexus Party", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", Keywords = "party group create host leader"))
	static UNexusCreatePartyProxy* CreateNexusParty(UObject* WorldContextObject, FNexusPartyHostParams Params);
	
	//~Begin UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~End UOnlineBlueprintCallProxyBase interface
	
private:
	UFUNCTION()
	virtual void OnPartyCreateResult(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState);
	
private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;
	
	FNexusPartyHostParams Params;
};
