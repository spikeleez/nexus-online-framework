// Copyright Spike Plugins 2026. All Rights Reserved.

#include "Proxy/NexusAcceptPartyInviteProxy.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusPartyManager.h"
#include "NexusLog.h"

UNexusAcceptPartyInviteProxy* UNexusAcceptPartyInviteProxy::AcceptNexusPartyInvite(UObject* WorldContextObject, const FNexusPendingPartyInvite& Invite)
{
	UNexusAcceptPartyInviteProxy* Proxy = NewObject<UNexusAcceptPartyInviteProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->Invite = Invite;
	return Proxy;
}

void UNexusAcceptPartyInviteProxy::Activate()
{
	if (!Invite.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Invalid invite — no lobby session data."));
		OnFailure.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
		SetReadyToDestroy();
		return;
	}

	const UNexusOnlineSubsystem* NexusSubsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!NexusSubsystem || !NexusSubsystem->GetPartyManager())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("NexusOnlineSubsystem or PartyManager unavailable."));
		OnFailure.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
		SetReadyToDestroy();
		return;
	}

	UNexusPartyManager* PartyManager = NexusSubsystem->GetPartyManager();
	PartyManager->OnPartyJoinedEvent.AddDynamic(this, &UNexusAcceptPartyInviteProxy::OnPartyJoinedResult);

	// JoinPartyFromSession handles:
	//   1. Deriving beacon address from the lobby session connect string
	//   2. Spawning + connecting the beacon client
	//   3. Sending ServerRequestJoin RPC
	//   4. Firing OnPartyJoinedEvent with the result
	if (!PartyManager->JoinPartyFromSession(Invite.PartyLobbySession))
	{
		UnbindFromManager();
		SetReadyToDestroy();
	}
}

void UNexusAcceptPartyInviteProxy::BeginDestroy()
{
	UnbindFromManager();
	Super::BeginDestroy();
}

void UNexusAcceptPartyInviteProxy::OnPartyJoinedResult(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState)
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

void UNexusAcceptPartyInviteProxy::UnbindFromManager()
{
	const UNexusOnlineSubsystem* NexusSubsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (NexusSubsystem && NexusSubsystem->GetPartyManager())
	{
		NexusSubsystem->GetPartyManager()->OnPartyJoinedEvent.RemoveDynamic(this, &UNexusAcceptPartyInviteProxy::OnPartyJoinedResult);
	}
}
