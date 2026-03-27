#include "Proxy/NexusLinkDestroySessionProxy.h"
#include "NexusLinkSubsystem.h"
#include "NexusLinkSessionManager.h"
#include "NexusLinkSettings.h"
#include "NexusLog.h"

UNexusLinkDestroySessionProxy* UNexusLinkDestroySessionProxy::DestroyNexusLinkSession(UObject* InWorldContextObject, FName InSessionName /*= NAME_None*/)
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
		return;
	}

	UNexusLinkSessionManager* SessionMgr = Subsystem->GetSessionManager();

	NativeDelegateHandle = SessionMgr->NativeOnSessionDestroyed.AddUObject(this, &ThisClass::OnDestroyComplete);

	if (!SessionMgr->DestroySession(SessionName))
	{
		Cleanup();
	}
}

void UNexusLinkDestroySessionProxy::BeginDestroy()
{
	Cleanup();
	Super::BeginDestroy();
}

void UNexusLinkDestroySessionProxy::OnDestroyComplete(ENexusLinkDestroySessionResult Result)
{
	Cleanup();

	if (Result == ENexusLinkDestroySessionResult::Success)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}
}

void UNexusLinkDestroySessionProxy::Cleanup()
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
}
