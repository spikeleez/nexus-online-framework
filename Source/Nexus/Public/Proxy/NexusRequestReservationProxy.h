#pragma once

#include "CoreMinimal.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "NexusOnlineTypes.h"
#include "NexusRequestReservationProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusReservationProxyComplete, ENexusReservationResult, Result);

/**
 * @class UNexusLinkRequestReservationProxy
 *
 * Async Blueprint node for requesting a party reservation in a session.
 * The proxy automatically unbinds from the manager when it completes or is destroyed.
 */
UCLASS()
class NEXUS_API UNexusRequestReservationProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

public:
	/** Fired when the server accepts the reservation. Now safe to call Join Session. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusReservationProxyComplete OnSuccess;

	/** Fired when the server rejects the reservation. Check Result for the reason. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events")
	FNexusReservationProxyComplete OnFailure;

public:
	/**
	 * Request a reservation in the target session.
	 *
	 * @param WorldContextObject World context.
	 * @param SearchResult The session to request a reservation in.
	 * @param Reservation Your party data (leader + members).
	 */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Reservations", DisplayName = "Request Nexus Reservation", meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "True", Keywords = "Party Reserve Slot Join"))
	static UNexusRequestReservationProxy* RequestNexusReservation(UObject* WorldContextObject, const FNexusSearchResult& SearchResult, const FNexusPartyReservation& Reservation);

	//~Begin UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	virtual void BeginDestroy() override;
	//~End UOnlineBlueprintCallProxyBase interface

private:
	UFUNCTION()
	void OnReservationResponse(ENexusReservationResult Result);

private:
	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FNexusSearchResult SearchResult;
	FNexusPartyReservation Reservation;
	FDelegateHandle NativeDelegateHandle;
};
