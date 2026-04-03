// Copyright Spike Plugins 2026. All Rights Reserved.

#include "Proxy/NexusCreatePartyProxy.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusPartyManager.h"
#include "NexusLog.h"

UNexusCreatePartyProxy* UNexusCreatePartyProxy::CreateNexusParty(UObject* WorldContextObject, int32 MaxSize)
{
	UNexusCreatePartyProxy* Proxy = NewObject<UNexusCreatePartyProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->MaxSize = MaxSize;
	
	return Proxy;
}

void UNexusCreatePartyProxy::Activate()
{
	const UNexusOnlineSubsystem* NexusOnlineSubsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!NexusOnlineSubsystem || !IsValid(NexusOnlineSubsystem->GetPartyManager()))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("NexusOnlineSubsystem or PartyManager unavailable."));
		OnFailure.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
		SetReadyToDestroy();
		return;
	}
	
	UNexusPartyManager* PartyManager = NexusOnlineSubsystem->GetPartyManager();
	
	// Bind before calling so the result is never missed, even if CreateParty fire synchronously.
	PartyManager->OnPartyCreatedEvent.AddDynamic(this, &UNexusCreatePartyProxy::OnPartyCreateResult);
	
	if (!PartyManager->CreateParty(MaxSize))
	{
		// CreateParty already broadcast (and triggered OnPartyCreatedResult) for the failure path.
		// Remove the binding only if it wasn't already cleaned up inside OnPartyCreatedResult.
		PartyManager->OnPartyCreatedEvent.RemoveDynamic(this, &UNexusCreatePartyProxy::OnPartyCreateResult);
		SetReadyToDestroy();
	}
}

void UNexusCreatePartyProxy::BeginDestroy()
{
	Super::BeginDestroy();
}

void UNexusCreatePartyProxy::OnPartyCreateResult(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState)
{
	// Unbind immediately — we only care about the first (and only) creation result
	const UNexusOnlineSubsystem* NexusOnlineSubsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (NexusOnlineSubsystem && IsValid(NexusOnlineSubsystem->GetPartyManager()))
	{
		NexusOnlineSubsystem->GetPartyManager()->OnPartyCreatedEvent.RemoveDynamic(this, &UNexusCreatePartyProxy::OnPartyCreateResult);
	}
	
	if (PartyResult == ENexusPartyResult::Success)
	{
		OnSuccess.Broadcast(PartyResult, PartyState);
	}
	else
	{
		OnFailure.Broadcast(PartyResult, PartyState);
	}
	
	SetReadyToDestroy();
}
