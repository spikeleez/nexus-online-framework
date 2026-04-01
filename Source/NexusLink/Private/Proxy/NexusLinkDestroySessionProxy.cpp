// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Proxy/NexusLinkDestroySessionProxy.h"
#include "NexusLinkSubsystem.h"
#include "NexusLinkSessionManager.h"
#include "NexusLinkSettings.h"
#include "NexusLog.h"

UNexusLinkDestroySessionProxy* UNexusLinkDestroySessionProxy::DestroyNexusSession(UObject* InWorldContextObject, FName InSessionName /*= NAME_None*/)
{
	UNexusLinkDestroySessionProxy* Proxy = NewObject<UNexusLinkDestroySessionProxy>();
	Proxy->WorldContextObject = InWorldContextObject;
	Proxy->SessionName = InSessionName.IsNone() ? UNexusLinkSettings::Get()->DefaultGameSessionName : InSessionName;

	return Proxy;
}

void UNexusLinkDestroySessionProxy::Activate()
{
	UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
	if (!Subsystem || !Subsystem->GetSessionManager())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("NexusLink subsystem or session manager unavailable."));
		OnFailure.Broadcast();
		SetReadyToDestroy();
		return;
	}

	UNexusLinkSessionManager* SessionManager = Subsystem->GetSessionManager();

	NativeDelegateHandle = SessionManager->NativeOnSessionDestroyed.AddUObject(this, &ThisClass::OnDestroyComplete);
	if (!SessionManager->DestroySession(SessionName))
	{
		OnFailure.Broadcast();
		SetReadyToDestroy();
	}
}

void UNexusLinkDestroySessionProxy::BeginDestroy()
{
	if (NativeDelegateHandle.IsValid())
	{
		UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetSessionManager())
		{
			Subsystem->GetSessionManager()->NativeOnSessionDestroyed.Remove(NativeDelegateHandle);
		}
		NativeDelegateHandle.Reset();
	}

	Super::BeginDestroy();
}

void UNexusLinkDestroySessionProxy::OnDestroyComplete(ENexusLinkDestroySessionResult Result)
{
	if (Result == ENexusLinkDestroySessionResult::Success)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}

	SetReadyToDestroy();
}