// Copyright Spike Plugins 2026. All Rights Reserved.

#include "Proxy/NexusJoinPartyProxy.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusPartyManager.h"
#include "NexusOnlineLibrary.h"
#include "NexusLog.h"

UNexusJoinPartyProxy* UNexusJoinPartyProxy::JoinNexusParty(UObject* WorldContextObject, const FNexusSearchResult& PartyLobbySession)
{
	UNexusJoinPartyProxy* Proxy = NewObject<UNexusJoinPartyProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->PartyLobbySession = PartyLobbySession;
	Proxy->bUseSessionJoin = true;
	return Proxy;
}

UNexusJoinPartyProxy* UNexusJoinPartyProxy::JoinNexusPartyDirect(UObject* WorldContextObject, const FString& HostAddress)
{
	UNexusJoinPartyProxy* Proxy = NewObject<UNexusJoinPartyProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->DirectAddress = HostAddress;
	Proxy->bUseSessionJoin = false;
	return Proxy;
}

void UNexusJoinPartyProxy::Activate()
{
	const UNexusOnlineSubsystem* NexusSubsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!NexusSubsystem || !NexusSubsystem->GetPartyManager())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("NexusOnlineSubsystem or PartyManager unavailable."));
		OnFailure.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
		SetReadyToDestroy();
		return;
	}

	UNexusPartyManager* PartyManager = NexusSubsystem->GetPartyManager();
	PartyManager->OnPartyJoinedEvent.AddDynamic(this, &UNexusJoinPartyProxy::OnPartyJoinedResult);

	bool bStarted = false;
	if (bUseSessionJoin)
	{
		bStarted = PartyManager->JoinPartyFromSession(PartyLobbySession);
	}
	else
	{
		ENexusBlueprintLibraryOutputResult IdResult;
		const FUniqueNetIdRepl LocalId = UNexusOnlineLibrary::GetLocalPlayerUniqueId(WorldContextObject, IdResult);
		if (!LocalId.IsValid())
		{
			OnFailure.Broadcast(ENexusPartyResult::InvalidState, FNexusPartyState());
			UnbindFromManager();
			SetReadyToDestroy();
			return;
		}

		ENexusBlueprintLibraryOutputResult NameResult;
		const FString LocalName = UNexusOnlineLibrary::GetLocalPlayerDisplayName(WorldContextObject, NameResult);
		bStarted = PartyManager->JoinParty(DirectAddress, LocalId, LocalName);
	}

	if (!bStarted)
	{
		UnbindFromManager();
		SetReadyToDestroy();
	}
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
	const UNexusOnlineSubsystem* NexusSubsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (NexusSubsystem && NexusSubsystem->GetPartyManager())
	{
		NexusSubsystem->GetPartyManager()->OnPartyJoinedEvent.RemoveDynamic(this, &UNexusJoinPartyProxy::OnPartyJoinedResult);
	}
}
