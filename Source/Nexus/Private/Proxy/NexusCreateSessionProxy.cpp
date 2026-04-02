// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Proxy/NexusCreateSessionProxy.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusSessionManager.h"
#include "NexusOnlineSettings.h"
#include "NexusLog.h"

UNexusCreateSessionProxy* UNexusCreateSessionProxy::CreateNexusSession(UObject* InWorldContextObject, const FNexuHostParams& InHostParams, FName InSessionName /*= NAME_None*/)
{
	UNexusCreateSessionProxy* Proxy = NewObject<UNexusCreateSessionProxy>();
	Proxy->WorldContextObject = InWorldContextObject;
	Proxy->HostParams = InHostParams;
	Proxy->SessionName = InSessionName.IsNone() ? UNexusOnlineSettings::Get()->DefaultGameSessionName : InSessionName;

	return Proxy;
}

void UNexusCreateSessionProxy::Activate()
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

	// Bind to native delegate so this proxy receives the completion callback.
	NativeDelegateHandle = SessionManager->NativeOnSessionCreated.AddUObject(this, &ThisClass::OnCreateComplete);

	bool bStartedSuccessfully = SessionManager->CreateSession(SessionName, HostParams);
	if (!bStartedSuccessfully)
	{
		// CreateSession returned false synchronously delegate was already fired inside CreateSession,
		// which means OnCreateComplete already ran. Just clean up in case it didn't.
		OnFailure.Broadcast();
		SetReadyToDestroy();
	}
}

void UNexusCreateSessionProxy::BeginDestroy()
{
	if (NativeDelegateHandle.IsValid())
	{
		UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetSessionManager())
		{
			Subsystem->GetSessionManager()->NativeOnSessionCreated.Remove(NativeDelegateHandle);
		}
		NativeDelegateHandle.Reset();
	}

	Super::BeginDestroy();
}

void UNexusCreateSessionProxy::OnCreateComplete(ENexusCreateSessionResult Result)
{
	if (Result == ENexusCreateSessionResult::Success)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}

	SetReadyToDestroy();
}