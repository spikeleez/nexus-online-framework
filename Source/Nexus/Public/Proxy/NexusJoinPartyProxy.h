// Copyright Spike Plugins 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusJoinPartyProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNexusJoinPartyProxyComplete, ENexusPartyResult, PartyResult, const FNexusPartyState&, PartyState);

/**
 * @class UNexusJoinPartyProxy
 *
 * Async Blueprint node for joining a party hosted by another player.
 * The result fires after the beacon connection + join handshake complete.
 */
UCLASS()
class NEXUS_API UNexusJoinPartyProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()
	
public:
	/** Fired when the join request is accepted by the party host. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusJoinPartyProxyComplete OnSuccess;

	/** Fired when the join request is rejected or the connection fails. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusJoinPartyProxyComplete OnFailure;
	
public:
	/**
	 * Join a party hosted by another player.
	 *
	 * @param WorldContextObject  World context.
	 * @param HostAddress         IP address of the party leader's beacon host.
	 * @param LocalPlayerId       The local player's unique net ID.
	 * @param DisplayName         Display name for the party roster.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party", DisplayName = "Join Nexus Party", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", Keywords = "party group join connect"))
	static UNexusJoinPartyProxy* JoinNexusParty(UObject* WorldContextObject, const FString& HostAddress, const FUniqueNetIdRepl& LocalPlayerId, const FString& DisplayName);
	
	//~Begin UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~End UOnlineBlueprintCallProxyBase interface
	
private:
	UFUNCTION()
	virtual void OnPartyJoinedResult(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState);

	void UnbindFromManager();
	
private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FString HostAddress;
	FString DisplayName;
	FUniqueNetIdRepl LocalPlayerId;
};
