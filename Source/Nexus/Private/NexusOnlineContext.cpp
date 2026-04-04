// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusOnlineContext.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "NexusLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusOnlineContext)

UWorld* UNexusOnlineContext::GetWorld() const
{
	if (CachedGameInstance.IsValid())
	{
		return CachedGameInstance->GetWorld();
	}
	return nullptr;
}

void UNexusOnlineContext::Initialize(UGameInstance* InGameInstance)
{
	check(InGameInstance);
	CachedGameInstance = InGameInstance;
	OnNexusInitialized();
}

void UNexusOnlineContext::Deinitialize()
{
	CachedGameInstance.Reset();
}

UGameInstance* UNexusOnlineContext::GetGameInstance() const
{
	return CachedGameInstance.Get();
}

void UNexusOnlineContext::OnSessionDestroyedForContext(ENexusDestroySessionResult Result)
{
	OnSessionLost();
}

void UNexusOnlineContext::OnPartyDisbandedForContext(ENexusPartyResult Reason)
{
	OnPartyDisbanded();
}

void UNexusOnlineContext::OnPartyMemberJoinedForContext(const FNexusPartySlot& NewMember, const FNexusPartyState& UpdatedState)
{
	OnPartyMemberJoined(NewMember);
}

void UNexusOnlineContext::OnPartyMemberLeftForContext(const FUniqueNetIdRepl& MemberId, ENexusPartyMemberStatus Reason)
{
	if (Reason == ENexusPartyMemberStatus::Kicked && !MemberId.IsValid())
	{
		// Invalid MemberId on Kicked means the LOCAL player was kicked (see PartyManager::OnClientKicked)
		OnKickedFromParty();
	}
	else
	{
		OnPartyMemberLeft(MemberId, Reason);
	}
}
