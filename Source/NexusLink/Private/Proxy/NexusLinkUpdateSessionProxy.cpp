#include "Proxy/NexusLinkUpdateSessionProxy.h"
#include "NexusLinkSubsystem.h"
#include "NexusLinkSessionManager.h"
#include "NexusLog.h"
#include "Engine/World.h"

UNexusLinkUpdateSessionProxy* UNexusLinkUpdateSessionProxy::UpdateNexusSession(UObject* WorldContextObject, FNexusLinkHostParams NewHostParams, FName SessionName)
{
	UNexusLinkUpdateSessionProxy* Proxy = NewObject<UNexusLinkUpdateSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = SessionName;
	Proxy->HostParams = NewHostParams;

	return Proxy;
}

void UNexusLinkUpdateSessionProxy::Activate()
{
	if (!WorldContextObject)
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("WorldContextObject invalid."));
		OnFailure.Broadcast(ENexusLinkUpdateSessionResult::Failure);
		SetReadyToDestroy();
		return;
	}

	UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Failed to get NexusLinkSubsystem"));
		OnFailure.Broadcast(ENexusLinkUpdateSessionResult::Failure);
		SetReadyToDestroy();
		return;
	}

	UNexusLinkSessionManager* SessionManager = Subsystem->GetSessionManager();
	if (!SessionManager)
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("SessionManager not found."));
		OnFailure.Broadcast(ENexusLinkUpdateSessionResult::Failure);
		SetReadyToDestroy();
		return;
	}

	DelegateHandle = SessionManager->NativeOnSessionUpdated.AddUObject(this, &UNexusLinkUpdateSessionProxy::OnSessionUpdatedCompleted);
	if (!SessionManager->UpdateSession(SessionName, HostParams))
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("The immediate call failed or was blocked (Spam Protection)."));
		OnFailure.Broadcast(ENexusLinkUpdateSessionResult::Failure);
		SetReadyToDestroy();
	}
}

void UNexusLinkUpdateSessionProxy::BeginDestroy()
{
	if (DelegateHandle.IsValid())
	{
		UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetSessionManager())
		{
			Subsystem->GetSessionManager()->NativeOnSessionUpdated.Remove(DelegateHandle);
		}
		DelegateHandle.Reset();
	}

	Super::BeginDestroy();
}

void UNexusLinkUpdateSessionProxy::OnSessionUpdatedCompleted(const ENexusLinkUpdateSessionResult Result)
{
	if (UNexusLinkSubsystem* Subsystem = UNexusLinkSubsystem::Get(WorldContextObject))
	{
		if (UNexusLinkSessionManager* SessionManager = Subsystem->GetSessionManager())
		{
			SessionManager->NativeOnSessionUpdated.Remove(DelegateHandle);
		}
	}

	if (Result == ENexusLinkUpdateSessionResult::Success)
	{
		OnSuccess.Broadcast(Result);
	}
	else
	{
		OnFailure.Broadcast(Result);
	}

	SetReadyToDestroy();
}
