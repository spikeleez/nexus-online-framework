#include "NexusLinkSubsystem.h"
#include "NexusLinkSessionManager.h"
#include "NexusLinkFriendManager.h"
#include "NexusLinkSettings.h"
#include "OnlineSubsystem.h"
#include "Engine/GameInstance.h"
#include "NexusLog.h"

UNexusLinkSubsystem::UNexusLinkSubsystem()
	: SessionManager(nullptr)
	, FriendManager(nullptr)
{

}

void UNexusLinkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameInstance* GameInstance = GetGameInstance();
	check(GameInstance);

	// Create and initialize managers. They are owned by this subsystem (Outer = this).
	SessionManager = NewObject<UNexusLinkSessionManager>(this);
	SessionManager->Initialize(GameInstance);

	FriendManager = NewObject<UNexusLinkFriendManager>(this);
	FriendManager->Initialize(GameInstance);

	NEXUS_LOG(LogNexusLink, Log, TEXT("Initialized. Online subsystem: %s"), *GetOnlineSubsystemName());
}

void UNexusLinkSubsystem::Deinitialize()
{
	if (SessionManager)
	{
		SessionManager->Deinitialize();
		SessionManager = nullptr;
	}

	if (FriendManager)
	{
		FriendManager->Deinitialize();
		FriendManager = nullptr;
	}

	NEXUS_LOG(LogNexusLink, Log, TEXT("Deinitialized."));
	Super::Deinitialize();
}

bool UNexusLinkSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Always create. Gate via settings in the future if needed.
	return true;
}

UNexusLinkSubsystem* UNexusLinkSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UNexusLinkSubsystem>();
}

bool UNexusLinkSubsystem::IsOnlineSubsystemAvailable()
{
	return IOnlineSubsystem::Get() != nullptr;
}

FString UNexusLinkSubsystem::GetOnlineSubsystemName()
{
	const IOnlineSubsystem* OnlineSubystem = IOnlineSubsystem::Get();
	if (!OnlineSubystem)
	{
		return TEXT("None");
	}
	return OnlineSubystem->GetSubsystemName().ToString();
}
