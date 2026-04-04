#include "Proxy/NexusUpdateSessionProxy.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusSessionManager.h"
#include "NexusLog.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusUpdateSessionProxy)

UNexusUpdateSessionProxy* UNexusUpdateSessionProxy::UpdateNexusSession(UObject* WorldContextObject, FNexusHostParams NewHostParams, FName SessionName)
{
	UNexusUpdateSessionProxy* Proxy = NewObject<UNexusUpdateSessionProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SessionName = SessionName;
	Proxy->HostParams = NewHostParams;

	return Proxy;
}

void UNexusUpdateSessionProxy::Activate()
{
	if (!WorldContextObject)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("WorldContextObject invalid."));
		OnFailure.Broadcast(ENexusUpdateSessionResult::Failure);
		SetReadyToDestroy();
		return;
	}

	UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed to get NexusSubsystem"));
		OnFailure.Broadcast(ENexusUpdateSessionResult::Failure);
		SetReadyToDestroy();
		return;
	}

	UNexusSessionManager* SessionManager = Subsystem->GetSessionManager();
	if (!SessionManager)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("SessionManager not found."));
		OnFailure.Broadcast(ENexusUpdateSessionResult::Failure);
		SetReadyToDestroy();
		return;
	}

	DelegateHandle = SessionManager->NativeOnSessionUpdated.AddUObject(this, &UNexusUpdateSessionProxy::OnSessionUpdatedCompleted);
	if (!SessionManager->UpdateSession(SessionName, HostParams))
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("The immediate call failed or was blocked (Spam Protection)."));
		OnFailure.Broadcast(ENexusUpdateSessionResult::Failure);
		SetReadyToDestroy();
	}
}

void UNexusUpdateSessionProxy::BeginDestroy()
{
	if (DelegateHandle.IsValid())
	{
		UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject);
		if (Subsystem && Subsystem->GetSessionManager())
		{
			Subsystem->GetSessionManager()->NativeOnSessionUpdated.Remove(DelegateHandle);
		}
		DelegateHandle.Reset();
	}

	Super::BeginDestroy();
}

void UNexusUpdateSessionProxy::OnSessionUpdatedCompleted(const ENexusUpdateSessionResult Result)
{
	if (UNexusOnlineSubsystem* Subsystem = UNexusOnlineSubsystem::Get(WorldContextObject))
	{
		if (UNexusSessionManager* SessionManager = Subsystem->GetSessionManager())
		{
			SessionManager->NativeOnSessionUpdated.Remove(DelegateHandle);
		}
	}

	if (Result == ENexusUpdateSessionResult::Success)
	{
		OnSuccess.Broadcast(Result);
	}
	else
	{
		OnFailure.Broadcast(Result);
	}

	SetReadyToDestroy();
}
