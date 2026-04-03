#include "Proxy/NexusRequestReservationProxy.h"
#include "NexusOnlineSubsystem.h"
#include "NexusLog.h"
#include "Managers/NexusReservationManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusRequestReservationProxy)

UNexusRequestReservationProxy* UNexusRequestReservationProxy::RequestNexusReservation(UObject* WorldContextObject, const FNexusSearchResult& SearchResult, const FNexusPartyReservation& Reservation)
{
	UNexusRequestReservationProxy* Proxy = NewObject<UNexusRequestReservationProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SearchResult = SearchResult;
	Proxy->Reservation = Reservation;
	return Proxy;
}

void UNexusRequestReservationProxy::Activate()
{
	UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!Subsystem || !Subsystem->GetReservationManager())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("NexusOnlineSubsystem or ReservationManager unavailable."));
		OnFailure.Broadcast(ENexusReservationResult::GeneralError);
		SetReadyToDestroy();
		return;
	}

	UNexusReservationManager* Manager = Subsystem->GetReservationManager();

	// Bind before sending so we never miss the response.
	NativeDelegateHandle = Manager->NativeOnReservationResponse.AddUObject(this, &UNexusRequestReservationProxy::OnReservationResponse);

	const APartyBeaconClient* Client = Manager->RequestReservation(SearchResult, Reservation);
	if (!Client)
	{
		// RequestReservation already logged the error.
		Manager->NativeOnReservationResponse.Remove(NativeDelegateHandle);
		NativeDelegateHandle.Reset();

		OnFailure.Broadcast(ENexusReservationResult::GeneralError);
		SetReadyToDestroy();
	}

	// Otherwise we wait for OnReservationResponse.
}

void UNexusRequestReservationProxy::BeginDestroy()
{
	if (NativeDelegateHandle.IsValid())
	{
		UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetReservationManager())
		{
			Subsystem->GetReservationManager()->NativeOnReservationResponse.Remove(NativeDelegateHandle);
		}
		NativeDelegateHandle.Reset();
	}

	Super::BeginDestroy();
}

void UNexusRequestReservationProxy::OnReservationResponse(ENexusReservationResult Result)
{
	if (Result == ENexusReservationResult::Success)
	{
		OnSuccess.Broadcast(Result);
	}
	else
	{
		OnFailure.Broadcast(Result);
	}

	SetReadyToDestroy();
}