// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Proxy/NexusDestroySessionProxy.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusSessionManager.h"
#include "NexusOnlineSettings.h"
#include "NexusLog.h"

UNexusDestroySessionProxy* UNexusDestroySessionProxy::DestroyNexusSession(UObject* InWorldContextObject, FName InSessionName /*= NAME_None*/)
{
	UNexusDestroySessionProxy* Proxy = NewObject<UNexusDestroySessionProxy>();
	Proxy->WorldContextObject = InWorldContextObject;
	Proxy->SessionName = InSessionName.IsNone() ? UNexusOnlineSettings::Get()->DefaultGameSessionName : InSessionName;

	return Proxy;
}

void UNexusDestroySessionProxy::Activate()
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

	NativeDelegateHandle = SessionManager->NativeOnSessionDestroyed.AddUObject(this, &ThisClass::OnDestroyComplete);
	if (!SessionManager->DestroySession(SessionName))
	{
		OnFailure.Broadcast();
		SetReadyToDestroy();
	}
}

void UNexusDestroySessionProxy::BeginDestroy()
{
	if (NativeDelegateHandle.IsValid())
	{
		UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetSessionManager())
		{
			Subsystem->GetSessionManager()->NativeOnSessionDestroyed.Remove(NativeDelegateHandle);
		}
		NativeDelegateHandle.Reset();
	}

	Super::BeginDestroy();
}

void UNexusDestroySessionProxy::OnDestroyComplete(ENexusDestroySessionResult Result)
{
	if (Result == ENexusDestroySessionResult::Success)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}

	SetReadyToDestroy();
}