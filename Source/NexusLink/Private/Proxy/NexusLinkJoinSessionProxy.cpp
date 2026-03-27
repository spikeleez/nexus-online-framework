// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Proxy/NexusLinkJoinSessionProxy.h"
#include "NexusLinkSubsystem.h"
#include "NexusLinkSessionManager.h"
#include "NexusLinkSettings.h"
#include "NexusLog.h"

UNexusLinkJoinSessionProxy* UNexusLinkJoinSessionProxy::JoinNexusLinkSession(UObject* InWorldContextObject, const FNexusLinkSearchResult& InSearchResult, FName InSessionName /*= NAME_None*/, const bool bInAutoTravel /*= true*/)
{
	UNexusLinkJoinSessionProxy* Proxy = NewObject<UNexusLinkJoinSessionProxy>();
	Proxy->WorldContextObject = InWorldContextObject;
	Proxy->SearchResult = InSearchResult;
	Proxy->SessionName = InSessionName.IsNone() ? UNexusLinkSettings::Get()->DefaultGameSessionName : InSessionName;
	Proxy->bAutoTravel = bInAutoTravel;

	return Proxy;
}

void UNexusLinkJoinSessionProxy::Activate()
{
	UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
	if (!Subsystem || !Subsystem->GetSessionManager())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("NexusLink subsystem or session manager unavailable."));
		OnFailure.Broadcast();
		return;
	}

	UNexusLinkSessionManager* SessionMgr = Subsystem->GetSessionManager();

	NativeDelegateHandle = SessionMgr->NativeOnSessionJoined.AddUObject(this, &ThisClass::OnJoinComplete);

	if (!SessionMgr->JoinSession(SessionName, SearchResult, bAutoTravel))
	{
		Cleanup();
	}
}

void UNexusLinkJoinSessionProxy::BeginDestroy()
{
	Cleanup();
	Super::BeginDestroy();
}

void UNexusLinkJoinSessionProxy::OnJoinComplete(ENexusLinkJoinSessionResult Result)
{
	Cleanup();

	if (Result == ENexusLinkJoinSessionResult::Success)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}
}

void UNexusLinkJoinSessionProxy::Cleanup()
{
	if (NativeDelegateHandle.IsValid())
	{
		UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetSessionManager())
		{
			Subsystem->GetSessionManager()->NativeOnSessionJoined.Remove(NativeDelegateHandle);
		}
		NativeDelegateHandle.Reset();
	}
}
