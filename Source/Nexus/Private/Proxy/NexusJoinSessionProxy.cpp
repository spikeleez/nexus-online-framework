// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Proxy/NexusJoinSessionProxy.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusSessionManager.h"
#include "NexusOnlineSettings.h"
#include "NexusLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusJoinSessionProxy)

UNexusJoinSessionProxy* UNexusJoinSessionProxy::JoinNexusSession(UObject* InWorldContextObject, const FNexusSearchResult& InSearchResult, FName InSessionName /*= NAME_None*/, const bool bInAutoTravel /*= true*/)
{
	UNexusJoinSessionProxy* Proxy = NewObject<UNexusJoinSessionProxy>();
	Proxy->WorldContextObject = InWorldContextObject;
	Proxy->SearchResult = InSearchResult;
	Proxy->SessionName = InSessionName.IsNone() ? UNexusOnlineSettings::Get()->DefaultGameSessionName : InSessionName;
	Proxy->bAutoTravel = bInAutoTravel;

	return Proxy;
}

void UNexusJoinSessionProxy::Activate()
{
	UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!Subsystem || !Subsystem->GetSessionManager())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Nexus subsystem or session manager unavailable."));
		OnFailure.Broadcast();
		SetReadyToDestroy();
		return;
	}

	UNexusSessionManager* SessionManager = Subsystem->GetSessionManager();
	NativeDelegateHandle = SessionManager->NativeOnSessionJoined.AddUObject(this, &ThisClass::OnJoinComplete);
	if (!SessionManager->JoinSession(SessionName, SearchResult, bAutoTravel))
	{
		OnFailure.Broadcast();
		SetReadyToDestroy();
	}
}

void UNexusJoinSessionProxy::BeginDestroy()
{
	if (NativeDelegateHandle.IsValid())
	{
		UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetSessionManager())
		{
			Subsystem->GetSessionManager()->NativeOnSessionJoined.Remove(NativeDelegateHandle);
		}
		NativeDelegateHandle.Reset();
	}

	Super::BeginDestroy();
}

void UNexusJoinSessionProxy::OnJoinComplete(ENexusJoinSessionResult Result)
{
	if (Result == ENexusJoinSessionResult::Success)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}

	SetReadyToDestroy();
}
