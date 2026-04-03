// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Managers/NexusFriendManager.h"
#include "NexusLog.h"
#include "NexusOnlineSettings.h"
#include "OnlineSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusFriendManager)

UNexusFriendManager::UNexusFriendManager()
	: bInviteHandlersRegistered(false)
{

}

void UNexusFriendManager::Initialize(UGameInstance* InGameInstance)
{
	check(InGameInstance);
	GameInstanceRef = InGameInstance;

	const UNexusOnlineSettings* Settings = UNexusOnlineSettings::Get();

	if (Settings->bAutoRegisterInviteHandlers)
	{
		RegisterInviteHandlers();
	}

	if (Settings->bAutoReadFriendsList)
	{
		ReadFriendsList();
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Initialized."));
}

void UNexusFriendManager::Deinitialize()
{
	UnregisterInviteHandlers();
	CachedFriends.Empty();
	PendingInvites.Empty();

	NEXUS_LOG(LogNexus, Log, TEXT("Deinitialized."));
}

bool UNexusFriendManager::ReadFriendsList()
{
	IOnlineFriendsPtr FriendsInterface = GetFriendsInterface();
	if (!FriendsInterface.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("No valid friends interface."));
		OnFriendsListReady.Broadcast(false, TArray<FNexusOnlineFriend>());
		return false;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Reading friends list..."));

	FriendsInterface->ReadFriendsList(
		0,
		EFriendsLists::ToString(EFriendsLists::Default),
		FOnReadFriendsListComplete::CreateUObject(this, &ThisClass::OnReadFriendsListComplete)
	);

	return true;
}

bool UNexusFriendManager::IsFriend(const FUniqueNetIdRepl& PlayerId) const
{
	if (!PlayerId.IsValid())
	{
		return false;
	}

	IOnlineFriendsPtr FriendsInterface = GetFriendsInterface();
	if (!FriendsInterface.IsValid())
	{
		return false;
	}

	return FriendsInterface->IsFriend(0, *PlayerId, EFriendsLists::ToString(EFriendsLists::Default));
}

bool UNexusFriendManager::SendSessionInvite(const FName SessionName, const FUniqueNetIdRepl& FriendId)
{
	if (!FriendId.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Invalid FriendId."));
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("No valid session interface."));
		return false;
	}

	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Could not get local player ID."));
		return false;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Sending invite for session '%s' to '%s'."),
		*SessionName.ToString(), *FriendId.ToString());

	return SessionInterface->SendSessionInviteToFriend(*LocalPlayerId, SessionName, *FriendId);
}

bool UNexusFriendManager::ShowPlatformInviteUI(const FName SessionName)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("No online subsystem."));
		return false;
	}

	IOnlineExternalUIPtr ExternalUI = OnlineSubsystem->GetExternalUIInterface();
	if (!ExternalUI.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("No external UI interface available."));
		return false;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Opening platform invite UI for session '%s'."), *SessionName.ToString());
	return ExternalUI->ShowInviteUI(0, SessionName);
}

void UNexusFriendManager::RegisterInviteHandlers()
{
	if (bInviteHandlersRegistered)
	{
		return;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("No valid session interface. Handlers not registered."));
		return;
	}

	OnSessionInviteReceivedDelegateHandle = SessionInterface->AddOnSessionInviteReceivedDelegate_Handle(
		FOnSessionInviteReceivedDelegate::CreateUObject(this, &ThisClass::OnSessionInviteReceivedInternal)
	);

	OnSessionUserInviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
		FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionUserInviteAcceptedInternal)
	);

	bInviteHandlersRegistered = true;
	NEXUS_LOG(LogNexus, Log, TEXT("Invite handlers registered."));
}

void UNexusFriendManager::UnregisterInviteHandlers()
{
	if (!bInviteHandlersRegistered)
	{
		return;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnSessionInviteReceivedDelegate_Handle(OnSessionInviteReceivedDelegateHandle);
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegateHandle);
	}

	bInviteHandlersRegistered = false;
	NEXUS_LOG(LogNexus, Log, TEXT("Invite handlers unregistered."));
}

IOnlineFriendsPtr UNexusFriendManager::GetFriendsInterface() const
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	return OnlineSub ? OnlineSub->GetFriendsInterface() : nullptr;
}

IOnlineSessionPtr UNexusFriendManager::GetSessionInterface() const
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	return OnlineSub ? OnlineSub->GetSessionInterface() : nullptr;
}

FUniqueNetIdPtr UNexusFriendManager::GetLocalPlayerId() const
{
	if (!GameInstanceRef.IsValid())
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = GameInstanceRef->GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		return nullptr;
	}

	return LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
}

FNexusOnlineFriend UNexusFriendManager::ConvertFriend(const TSharedRef<FOnlineFriend>& InFriend)
{
	// Dereference the shared ref to call our FOnlineFriend constructor.
	return FNexusOnlineFriend(*InFriend);
}

void UNexusFriendManager::OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	CachedFriends.Empty();

	if (!bWasSuccessful)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Failed. Error: %s"), *ErrorStr);
		OnFriendsListReady.Broadcast(false, CachedFriends);
		return;
	}

	IOnlineFriendsPtr FriendsInterface = GetFriendsInterface();
	if (!FriendsInterface.IsValid())
	{
		NEXUS_LOG(LogNexus, Error, TEXT("Friends interface became invalid during callback."));
		OnFriendsListReady.Broadcast(false, CachedFriends);
		return;
	}

	TArray<TSharedRef<FOnlineFriend>> OnlineFriends;
	FriendsInterface->GetFriendsList(LocalUserNum, ListName, OnlineFriends);

	CachedFriends.Reserve(OnlineFriends.Num());
	for (const TSharedRef<FOnlineFriend>& Friend : OnlineFriends)
	{
		CachedFriends.Emplace(ConvertFriend(Friend));
	}

	NEXUS_LOG(LogNexus, Log, TEXT("Loaded %d friend(s)."), CachedFriends.Num());
	OnFriendsListReady.Broadcast(true, CachedFriends);
}

void UNexusFriendManager::OnSessionInviteReceivedInternal(const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FString& AppId, const FOnlineSessionSearchResult& InviteResult)
{
	NEXUS_LOG(LogNexus, Log, TEXT("From %s"), *FromId.ToString());

	FNexusPendingInvite NewInvite;
	NewInvite.FromId = FUniqueNetIdRepl(FromId.AsShared());
	NewInvite.Session = FNexusSearchResult(InviteResult);
	NewInvite.TimeReceived = FPlatformTime::Seconds();

	PendingInvites.Add(NewInvite);
	OnSessionInviteReceived.Broadcast(NewInvite);
}

void UNexusFriendManager::OnSessionUserInviteAcceptedInternal(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("Platform reported invite acceptance failed."));
		return;
	}

	NEXUS_LOG(LogNexus, Log, TEXT("User accepted a session invite via platform UI."));

	FNexusSearchResult WrappedResult(InviteResult);
	OnSessionInviteAccepted.Broadcast(WrappedResult);
}
