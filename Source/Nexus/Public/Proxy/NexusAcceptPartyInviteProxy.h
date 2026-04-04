// Copyright Spike Plugins 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NexusOnlineTypes.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusAcceptPartyInviteProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNexusAcceptPartyInviteProxyComplete, ENexusPartyResult, PartyResult, const FNexusPartyState&, PartyState);

/**
 * @class UNexusAcceptPartyInviteProxy
 *
 * Async Blueprint node for accepting a party invite received via platform overlay.
 *
 * Internally handles the full flow:
 *   1. Joins the party leader's lobby session (OSS)
 *   2. Derives the beacon address
 *   3. Connects to the party beacon
 *
 * The caller only needs the FNexusPendingPartyInvite received from
 * UNexusOnlineContext::OnPartyInviteReceived (or via the NexusPartyListenerComponent).
 *
 * Blueprint pattern:
 *   OnPartyInviteReceived -> Show notification -> [Button Aceitar]
 *   -> Accept Nexus Party Invite(Invite)
 *      OnSuccess -> Show party lobby UI
 *      OnFailure -> Show error message
 */
UCLASS()
class NEXUS_API UNexusAcceptPartyInviteProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the join is accepted by the party beacon and the party state is available. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusAcceptPartyInviteProxyComplete OnSuccess;

	/** Fired when the join fails for any reason (session full, connection error, etc.). */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusAcceptPartyInviteProxyComplete OnFailure;

public:
	/**
	 * Accept a party invite. No other setup is required.
	 *
	 * @param WorldContextObject World context.
	 * @param Invite The invite received from OnPartyInviteReceived.
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Party", DisplayName = "Accept Nexus Party Invite", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", Keywords = "party accept invite join"))
	static UNexusAcceptPartyInviteProxy* AcceptNexusPartyInvite(UObject* WorldContextObject, const FNexusPendingPartyInvite& Invite);

	//~Begin UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~End UOnlineBlueprintCallProxyBase interface

private:
	UFUNCTION()
	void OnPartyJoinedResult(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState);

	void UnbindFromManager();

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FNexusPendingPartyInvite Invite;
};
