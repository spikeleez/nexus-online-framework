// Copyright Spike Plugins 2026. All Rights Reserved.

#include "Proxy/NexusJoinPartyProxy.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusPartyManager.h"

UNexusJoinPartyProxy* UNexusJoinPartyProxy::JoinNexusParty(UObject* WorldContextObject, const FString& HostAddress, const FUniqueNetIdRepl& LocalPlayerId, const FString& DisplayName)
{
	UNexusJoinPartyProxy* Proxy = NewObject<UNexusJoinPartyProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->HostAddress = HostAddress;
	Proxy->LocalPlayerId = LocalPlayerId;
	Proxy->DisplayName = DisplayName;
	
	return Proxy;
}

void UNexusJoinPartyProxy::Activate()
{
	const UNexusOnlineSubsystem* NexusOnlineSubsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!NexusOnlineSubsystem || !NexusOnlineSubsystem->GetPartyManager())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("NexusOnlineSubsystem or PartyManager unavailable."));
		OnFailure.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
		SetReadyToDestroy();
		return;
	}

	UNexusPartyManager* PartyManager = NexusOnlineSubsystem->GetPartyManager();
	PartyManager->OnPartyJoinedEvent.AddDynamic(this, &UNexusJoinPartyProxy::OnPartyJoinedResult);

	if (!PartyManager->JoinParty(HostAddress, LocalPlayerId, DisplayName))
	{
		// JoinParty already broadcast a failure; OnPartyJoinedResult may have already fired.
		UnbindFromManager();
		SetReadyToDestroy();
	}
	// Otherwise await the async OnPartyJoinedResult callback
}

void UNexusJoinPartyProxy::BeginDestroy()
{
	UnbindFromManager();
	
	Super::BeginDestroy();
}

void UNexusJoinPartyProxy::OnPartyJoinedResult(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState)
{
	UnbindFromManager();

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

void UNexusJoinPartyProxy::UnbindFromManager()
{
	const UNexusOnlineSubsystem* NexusOnlineSubsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (NexusOnlineSubsystem && NexusOnlineSubsystem->GetPartyManager())
	{
		NexusOnlineSubsystem->GetPartyManager()->OnPartyJoinedEvent.RemoveDynamic(this, &UNexusJoinPartyProxy::OnPartyJoinedResult);
	}
}
