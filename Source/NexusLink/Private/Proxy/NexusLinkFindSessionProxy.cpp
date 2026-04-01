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
		SetReadyToDestroy();
		return;
	}

	UNexusLinkSessionManager* SessionManager = Subsystem->GetSessionManager();

	// Bind to native delegate so this proxy receives the completion callback.
	NativeDelegateHandle = SessionManager->NativeOnSessionsFound.AddUObject(this, &ThisClass::OnFindComplete);

	bool bFoundSuccessfully = SessionManager->FindSessions(SearchParams);
	if (!bFoundSuccessfully)
	{
		// FindSession returned false synchronously — delegate was already fired inside FindSession,
		// which means OnFindComplete already ran. Just clean up in case it didn't.
		OnFailure.Broadcast(TArray<FNexusLinkSearchResult>());
		SetReadyToDestroy();
	}
}

void UNexusLinkFindSessionProxy::BeginDestroy()
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

	Super::BeginDestroy();
}

void UNexusLinkFindSessionProxy::OnFindComplete(ENexusLinkFindSessionsResult Result, const TArray<FNexusLinkSearchResult>& Results)
{
	if (Result == ENexusLinkFindSessionsResult::Success || Result == ENexusLinkFindSessionsResult::NoResults)
	{
		OnSuccess.Broadcast(Results);
	}
	else
	{
		OnFailure.Broadcast(Results);
	}

	SetReadyToDestroy();
}
