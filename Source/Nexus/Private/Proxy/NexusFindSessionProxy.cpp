// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Proxy/NexusFindSessionProxy.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusSessionManager.h"
#include "NexusLog.h"

UNexusFindSessionProxy* UNexusFindSessionProxy::FindNexusSessions(UObject* InWorldContextObject, const FNexusSearchParams& InSearchParams)
{
	UNexusFindSessionProxy* Proxy = NewObject<UNexusFindSessionProxy>();
	Proxy->WorldContextObject = InWorldContextObject;
	Proxy->SearchParams = InSearchParams;
	return Proxy;
}

void UNexusFindSessionProxy::Activate()
{
	UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!Subsystem || !Subsystem->GetSessionManager())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Nexus subsystem or session manager unavailable."));
		OnFailure.Broadcast(TArray<FNexusSearchResult>());
		SetReadyToDestroy();
		return;
	}

	UNexusSessionManager* SessionManager = Subsystem->GetSessionManager();

	// Bind to native delegate so this proxy receives the completion callback.
	NativeDelegateHandle = SessionManager->NativeOnSessionsFound.AddUObject(this, &ThisClass::OnFindComplete);

	bool bFoundSuccessfully = SessionManager->FindSessions(SearchParams);
	if (!bFoundSuccessfully)
	{
		// FindSession returned false synchronously delegate was already fired inside FindSession,
		// which means OnFindComplete already ran. Just clean up in case it didn't.
		OnFailure.Broadcast(TArray<FNexusSearchResult>());
		SetReadyToDestroy();
	}
}

void UNexusFindSessionProxy::BeginDestroy()
{
	if (NativeDelegateHandle.IsValid())
	{
		UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetSessionManager())
		{
			Subsystem->GetSessionManager()->NativeOnSessionsFound.Remove(NativeDelegateHandle);
		}
		NativeDelegateHandle.Reset();
	}

	Super::BeginDestroy();
}

void UNexusFindSessionProxy::OnFindComplete(ENexusFindSessionsResult Result, const TArray<FNexusSearchResult>& Results)
{
	if (Result == ENexusFindSessionsResult::Success || Result == ENexusFindSessionsResult::NoResults)
	{
		OnSuccess.Broadcast(Results);
	}
	else
	{
		OnFailure.Broadcast(Results);
	}

	SetReadyToDestroy();
}
