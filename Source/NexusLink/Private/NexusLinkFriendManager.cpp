#include "NexusLinkFriendManager.h"
#include "NexusLog.h"
#include "NexusLinkSettings.h"
#include "OnlineSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"

UNexusLinkFriendManager::UNexusLinkFriendManager()
	: bInviteHandlersRegistered(false)
{

}

void UNexusLinkFriendManager::Initialize(UGameInstance* InGameInstance)
{
	check(InGameInstance);
	GameInstanceRef = InGameInstance;

	const UNexusLinkSettings* Settings = UNexusLinkSettings::Get();

	if (Settings->bAutoRegisterInviteHandlers)
	{
		RegisterInviteHandlers();
	}

	if (Settings->bAutoReadFriendsList)
	{
		ReadFriendsList();
	}

	NEXUS_LOG(LogNexusLink, Log, TEXT("Initialized."));
}

void UNexusLinkFriendManager::Deinitialize()
{
	UnregisterInviteHandlers();
	CachedFriends.Empty();
	PendingInvites.Empty();

	NEXUS_LOG(LogNexusLink, Log, TEXT("Deinitialized."));
}

bool UNexusLinkFriendManager::ReadFriendsList()
{
	IOnlineFriendsPtr FriendsInterface = GetFriendsInterface();
	if (!FriendsInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No valid friends interface."));
		OnFriendsListReady.Broadcast(false, TArray<FNexusLinkOnlineFriend>());
		return false;
	}

	NEXUS_LOG(LogNexusLink, Log, TEXT("Reading friends list..."));

	FriendsInterface->ReadFriendsList(
		0,
		EFriendsLists::ToString(EFriendsLists::Default),
		FOnReadFriendsListComplete::CreateUObject(this, &ThisClass::OnReadFriendsListComplete)
	);

	return true;
}

bool UNexusLinkFriendManager::IsFriend(const FUniqueNetIdRepl& PlayerId) const
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

bool UNexusLinkFriendManager::SendSessionInvite(const FName SessionName, const FUniqueNetIdRepl& FriendId)
{
	if (!FriendId.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Invalid FriendId."));
		return false;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No valid session interface."));
		return false;
	}

	FUniqueNetIdPtr LocalPlayerId = GetLocalPlayerId();
	if (!LocalPlayerId.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Could not get local player ID."));
		return false;
	}

	NEXUS_LOG(LogNexusLink, Log, TEXT("Sending invite for session '%s' to '%s'."),
		*SessionName.ToString(), *FriendId.ToString());

	return SessionInterface->SendSessionInviteToFriend(*LocalPlayerId, SessionName, *FriendId);
}

bool UNexusLinkFriendManager::ShowPlatformInviteUI(const FName SessionName)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem)
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No online subsystem."));
		return false;
	}

	IOnlineExternalUIPtr ExternalUI = OnlineSubsystem->GetExternalUIInterface();
	if (!ExternalUI.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("No external UI interface available."));
		return false;
	}

	NEXUS_LOG(LogNexusLink, Log, TEXT("Opening platform invite UI for session '%s'."), *SessionName.ToString());
	return ExternalUI->ShowInviteUI(0, SessionName);
}

void UNexusLinkFriendManager::RegisterInviteHandlers()
{
	if (bInviteHandlersRegistered)
	{
		return;
	}

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("No valid session interface. Handlers not registered."));
		return;
	}

	OnSessionInviteReceivedDelegateHandle = SessionInterface->AddOnSessionInviteReceivedDelegate_Handle(
		FOnSessionInviteReceivedDelegate::CreateUObject(this, &ThisClass::OnSessionInviteReceivedInternal)
	);

	OnSessionUserInviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
		FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionUserInviteAcceptedInternal)
	);

	bInviteHandlersRegistered = true;
	NEXUS_LOG(LogNexusLink, Log, TEXT("Invite handlers registered."));
}

void UNexusLinkFriendManager::UnregisterInviteHandlers()
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
	NEXUS_LOG(LogNexusLink, Log, TEXT("Invite handlers unregistered."));
}

IOnlineFriendsPtr UNexusLinkFriendManager::GetFriendsInterface() const
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	return OnlineSub ? OnlineSub->GetFriendsInterface() : nullptr;
}

IOnlineSessionPtr UNexusLinkFriendManager::GetSessionInterface() const
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	return OnlineSub ? OnlineSub->GetSessionInterface() : nullptr;
}

FUniqueNetIdPtr UNexusLinkFriendManager::GetLocalPlayerId() const
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

FNexusLinkOnlineFriend UNexusLinkFriendManager::ConvertFriend(const TSharedRef<FOnlineFriend>& InFriend)
{
	// Dereference the shared ref to call our FOnlineFriend constructor.
	return FNexusLinkOnlineFriend(*InFriend);
}

void UNexusLinkFriendManager::OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	CachedFriends.Empty();

	if (!bWasSuccessful)
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Failed. Error: %s"), *ErrorStr);
		OnFriendsListReady.Broadcast(false, CachedFriends);
		return;
	}

	IOnlineFriendsPtr FriendsInterface = GetFriendsInterface();
	if (!FriendsInterface.IsValid())
	{
		NEXUS_LOG(LogNexusLink, Error, TEXT("Friends interface became invalid during callback."));
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

	NEXUS_LOG(LogNexusLink, Log, TEXT("Loaded %d friend(s)."), CachedFriends.Num());
	OnFriendsListReady.Broadcast(true, CachedFriends);
}

void UNexusLinkFriendManager::OnSessionInviteReceivedInternal(const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FString& AppId, const FOnlineSessionSearchResult& InviteResult)
{
	NEXUS_LOG(LogNexusLink, Log, TEXT("From %s"), *FromId.ToString());

	FNexusLinkPendingInvite NewInvite;
	NewInvite.FromId = FUniqueNetIdRepl(FromId.AsShared());
	NewInvite.Session = FNexusLinkSearchResult(InviteResult);
	NewInvite.TimeReceived = FPlatformTime::Seconds();

	PendingInvites.Add(NewInvite);
	OnSessionInviteReceived.Broadcast(NewInvite);
}

void UNexusLinkFriendManager::OnSessionUserInviteAcceptedInternal(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful)
	{
		NEXUS_LOG(LogNexusLink, Warning, TEXT("Platform reported invite acceptance failed."));
		return;
	}

	NEXUS_LOG(LogNexusLink, Log, TEXT("User accepted a session invite via platform UI."));

	FNexusLinkSearchResult WrappedResult(InviteResult);
	OnSessionInviteAccepted.Broadcast(WrappedResult);
}
