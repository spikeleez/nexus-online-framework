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
 * Async Blueprint node for creating a party.
 * Although CreateParty is synchronous internally, this proxy provides
 * a consistent latent-node API alongside the async JoinParty proxy.
 */
UCLASS()
class NEXUS_API UNexusCreatePartyProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()
	
public:
	/** Fired when the party is created successfully. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusCreatePartyProxyComplete OnSuccess;
	
	/** Fired when party creation fails. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusCreatePartyProxyComplete OnFailure;
	
public:
	/**
	 * Create a party. The local player becomes the party leader.
	 *
	 * @param WorldContextObject  World context.
	 * @param MaxSize             Maximum party size including the leader (clamped to [2, 8]).
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party", DisplayName = "Create Nexus Party", meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", Keywords = "party group create host"))
	static UNexusCreatePartyProxy* CreateNexusParty(UObject* WorldContextObject, int32 MaxSize = 4);
	
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
	
	int32 MaxSize = 4;
};
