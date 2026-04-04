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
 *
 * Prefer JoinNexusParty (from SearchResult) whenever you have a party lobby session.
 * Use JoinNexusPartyDirect only for LAN / direct-IP scenarios.
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
	 * Join a party using the party lobby session (recommended).
	 * The beacon address is derived automatically — no manual IP needed.
	 *
	 * @param WorldContextObject World context.
	 * @param PartyLobbySession The party's lobby session search result.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party", DisplayName = "Join Nexus Party", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", Keywords = "party group join connect"))
	static UNexusJoinPartyProxy* JoinNexusParty(UObject* WorldContextObject, const FNexusSearchResult& PartyLobbySession);

	/**
	 * Join a party via a raw beacon address string.
	 * Use only for LAN / direct-IP scenarios where you already know the host address.
	 *
	 * @param WorldContextObject World context.
	 * @param HostAddress "IP:Port" of the party leader's beacon host.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party", DisplayName = "Join Nexus Party (Direct Address)", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", Keywords = "party group join direct ip address lan"))
	static UNexusJoinPartyProxy* JoinNexusPartyDirect(UObject* WorldContextObject, const FString& HostAddress);
	
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

	FNexusSearchResult PartyLobbySession;
	FString DirectAddress;
	bool bUseSessionJoin = true;
};
