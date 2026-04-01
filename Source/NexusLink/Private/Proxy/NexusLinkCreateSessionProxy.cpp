// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Proxy/NexusLinkCreateSessionProxy.h"
#include "NexusLinkSubsystem.h"
#include "NexusLinkSessionManager.h"
#include "NexusLinkSettings.h"
#include "NexusLog.h"

UNexusLinkCreateSessionProxy* UNexusLinkCreateSessionProxy::CreateNexusLinkSession(UObject* InWorldContextObject, const FNexusLinkHostParams& InHostParams, FName InSessionName /*= NAME_None*/)
{
	UNexusLinkCreateSessionProxy* Proxy = NewObject<UNexusLinkCreateSessionProxy>();
	Proxy->WorldContextObject = InWorldContextObject;
	Proxy->HostParams = InHostParams;
	Proxy->SessionName = InSessionName.IsNone() ? UNexusLinkSettings::Get()->DefaultGameSessionName : InSessionName;

	return Proxy;
}

void UNexusLinkCreateSessionProxy::Activate()
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

	// Bind to native delegate so this proxy receives the completion callback.
	NativeDelegateHandle = SessionManager->NativeOnSessionCreated.AddUObject(this, &ThisClass::OnCreateComplete);

	bool bStartedSuccessfully = SessionManager->CreateSession(SessionName, HostParams);
	if (!bStartedSuccessfully)
	{
		// CreateSession returned false synchronously — delegate was already fired inside CreateSession,
		// which means OnCreateComplete already ran. Just clean up in case it didn't.
		OnFailure.Broadcast();
		SetReadyToDestroy();
	}
}

void UNexusLinkCreateSessionProxy::BeginDestroy()
{
	if (NativeDelegateHandle.IsValid())
	{
		UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetSessionManager())
		{
			Subsystem->GetSessionManager()->NativeOnSessionCreated.Remove(NativeDelegateHandle);
		}
		NativeDelegateHandle.Reset();
	}

	Super::BeginDestroy();
}

void UNexusLinkCreateSessionProxy::OnCreateComplete(ENexusLinkCreateSessionResult Result)
{
	if (Result == ENexusLinkCreateSessionResult::Success)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}

	SetReadyToDestroy();
}