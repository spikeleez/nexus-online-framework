// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Proxy/NexusLinkFindSessionProxy.h"
#include "NexusLinkSubsystem.h"
#include "NexusLinkSessionManager.h"
#include "NexusLog.h"

UNexusLinkFindSessionProxy* UNexusLinkFindSessionProxy::FindNexusLinkSessions(UObject* InWorldContextObject, const FNexusLinkSearchParams& InSearchParams)
{
	UNexusLinkFindSessionProxy* Proxy = NewObject<UNexusLinkFindSessionProxy>();
	Proxy->WorldContextObject = InWorldContextObject;
	Proxy->SearchParams = InSearchParams;
	return Proxy;
}

void UNexusLinkFindSessionProxy::Activate()
{
	UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
	if (!Subsystem || !Subsystem->GetSessionManager())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("NexusLink subsystem or session manager unavailable."));
		OnFailure.Broadcast(TArray<FNexusLinkSearchResult>());
		return;
	}

	UNexusLinkSessionManager* SessionMgr = Subsystem->GetSessionManager();

	NativeDelegateHandle = SessionMgr->NativeOnSessionsFound.AddUObject(this, &ThisClass::OnFindComplete);

	if (!SessionMgr->FindSessions(SearchParams))
	{
		Cleanup();
	}
}

void UNexusLinkFindSessionProxy::BeginDestroy()
{
	Cleanup();
	Super::BeginDestroy();
}

void UNexusLinkFindSessionProxy::OnFindComplete(ENexusLinkFindSessionsResult Result, const TArray<FNexusLinkSearchResult>& Results)
{
	Cleanup();

	if (Result == ENexusLinkFindSessionsResult::Success || Result == ENexusLinkFindSessionsResult::NoResults)
	{
		OnSuccess.Broadcast(Results);
	}
	else
	{
		OnFailure.Broadcast(Results);
	}
}

void UNexusLinkFindSessionProxy::Cleanup()
{
	if (NativeDelegateHandle.IsValid())
	{
		UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetSessionManager())
		{
			Subsystem->GetSessionManager()->NativeOnSessionsFound.Remove(NativeDelegateHandle);
		}
		NativeDelegateHandle.Reset();
	}
}
