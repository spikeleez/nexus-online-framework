// Copyright Spike Plugins 2026. All Rights Reserved.

#include "Proxy/NexusCreatePartyProxy.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusPartyManager.h"
#include "NexusLog.h"

UNexusCreatePartyProxy* UNexusCreatePartyProxy::CreateNexusParty(UObject* WorldContextObject, FNexusPartyHostParams Params)
{
	UNexusCreatePartyProxy* Proxy = NewObject<UNexusCreatePartyProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->Params = Params;
	return Proxy;
}

void UNexusCreatePartyProxy::Activate()
{
	const UNexusOnlineSubsystem* NexusSubsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!NexusSubsystem || !IsValid(NexusSubsystem->GetPartyManager()))
	{
		NEXUS_LOG(LogNexus, Error, TEXT("NexusOnlineSubsystem or PartyManager unavailable."));
		OnFailure.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
		SetReadyToDestroy();
		return;
	}
	
	UNexusPartyManager* PartyManager = NexusSubsystem->GetPartyManager();
	
	// Bind before calling so the result is never missed, even if CreateParty fires synchronously.
	PartyManager->OnPartyCreatedEvent.AddDynamic(this, &UNexusCreatePartyProxy::OnPartyCreateResult);
	
	if (!PartyManager->CreateParty(Params))
	{
		PartyManager->OnPartyCreatedEvent.RemoveDynamic(this, &UNexusCreatePartyProxy::OnPartyCreateResult);
		SetReadyToDestroy();
	}
	// Otherwise wait for the async lobby session creation — OnPartyCreateResult fires on completion.
}

void UNexusCreatePartyProxy::BeginDestroy()
{
	Super::BeginDestroy();
}

void UNexusCreatePartyProxy::OnPartyCreateResult(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState)
{
	const UNexusOnlineSubsystem* NexusSubsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (NexusSubsystem && IsValid(NexusSubsystem->GetPartyManager()))
	{
		NexusSubsystem->GetPartyManager()->OnPartyCreatedEvent.RemoveDynamic(this, &UNexusCreatePartyProxy::OnPartyCreateResult);
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
