// Copyright Spike Plugins 2026. All Rights Reserved.

#include "Components/NexusPartyListenerComponent.h"
#include "NexusOnlineSubsystem.h"
#include "Managers/NexusPartyManager.h"
#include "NexusLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusPartyListenerComponent)

UNexusPartyListenerComponent::UNexusPartyListenerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsBound(false)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	bWantsInitializeComponent = false;
}

void UNexusPartyListenerComponent::BeginPlay()
{
	BindToManagers();
	Super::BeginPlay();
}

void UNexusPartyListenerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromManagers();
	Super::EndPlay(EndPlayReason);
}

void UNexusPartyListenerComponent::BindToManagers()
{
	if (bIsBound)
	{
		return;
	}
	
	const UNexusOnlineSubsystem* NexusOnlineSubsystem = UNexusOnlineSubsystem::Get(this);
	if (!NexusOnlineSubsystem)
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("NexusOnlineSubsystem unavailable. Party delegates will not be forwarded."));
		return;
	}
	
	UNexusPartyManager* PartyManager = NexusOnlineSubsystem->GetPartyManager();
	if (!IsValid(PartyManager))
	{
		NEXUS_LOG(LogNexus, Warning, TEXT("PartyManager is invalid."));
		return;
	}
	
	PartyManager->OnPartyCreatedEvent.AddDynamic(this, &UNexusPartyListenerComponent::OnPartyCreated);
	PartyManager->OnPartyJoinedEvent.AddDynamic(this, &UNexusPartyListenerComponent::OnPartyJoined);
	PartyManager->OnPartyStateUpdatedEvent.AddDynamic(this, &UNexusPartyListenerComponent::OnPartyStateUpdated);
	PartyManager->OnPartyMemberJoinedEvent.AddDynamic(this, &UNexusPartyListenerComponent::OnPartyMemberJoined);
	PartyManager->OnPartyMemberLeftEvent.AddDynamic(this, &UNexusPartyListenerComponent::OnPartyMemberLeft);
	PartyManager->OnPartyDisbandedEvent.AddDynamic(this, &UNexusPartyListenerComponent::OnPartyDisbanded);
	PartyManager->OnPartyInviteReceivedEvent.AddDynamic(this, &UNexusPartyListenerComponent::OnPartyInviteReceived);
	PartyManager->OnPartyGameSessionReadyEvent.AddDynamic(this, &UNexusPartyListenerComponent::OnPartyGameSessionReady);
	
	bIsBound = true;
	NEXUS_LOG(LogNexus, Verbose, TEXT("Bound to party delegates."));
}

void UNexusPartyListenerComponent::UnbindFromManagers()
{
	if (!bIsBound)
	{
		return;
	}

	if (const UNexusOnlineSubsystem* NexusOnlineSubsystem = UNexusOnlineSubsystem::Get(this))
	{
		if (UNexusPartyManager* PartyManager = NexusOnlineSubsystem->GetPartyManager())
		{
			PartyManager->OnPartyCreatedEvent.RemoveDynamic(this, &ThisClass::OnPartyCreated);
			PartyManager->OnPartyJoinedEvent.RemoveDynamic(this, &ThisClass::OnPartyJoined);
			PartyManager->OnPartyStateUpdatedEvent.RemoveDynamic(this, &ThisClass::OnPartyStateUpdated);
			PartyManager->OnPartyMemberJoinedEvent.RemoveDynamic(this, &ThisClass::OnPartyMemberJoined);
			PartyManager->OnPartyMemberLeftEvent.RemoveDynamic(this, &ThisClass::OnPartyMemberLeft);
			PartyManager->OnPartyDisbandedEvent.RemoveDynamic(this, &ThisClass::OnPartyDisbanded);
			PartyManager->OnPartyInviteReceivedEvent.RemoveDynamic(this, &ThisClass::OnPartyInviteReceived);
			PartyManager->OnPartyGameSessionReadyEvent.RemoveDynamic(this, &ThisClass::OnPartyGameSessionReady);
		}
	}

	bIsBound = false;
	NEXUS_LOG(LogNexus, Verbose, TEXT("Unbound from party delegates."));
}

void UNexusPartyListenerComponent::OnPartyCreated(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState)
{
	OnPartyCreatedEvent.Broadcast(PartyResult, PartyState);
}

void UNexusPartyListenerComponent::OnPartyJoined(ENexusPartyResult PartyResult, const FNexusPartyState& PartyState)
{
	OnPartyJoinedEvent.Broadcast(PartyResult, PartyState);
}

void UNexusPartyListenerComponent::OnPartyStateUpdated(const FNexusPartyState& PartyState)
{
	OnPartyStateUpdatedEvent.Broadcast(PartyState);
}

void UNexusPartyListenerComponent::OnPartyMemberJoined(const FNexusPartySlot& NewMember, const FNexusPartyState& UpdatedState)
{
	OnPartyMemberJoinedEvent.Broadcast(NewMember, UpdatedState);
}

void UNexusPartyListenerComponent::OnPartyMemberLeft(const FUniqueNetIdRepl& MemberId, ENexusPartyMemberStatus MemberStatus)
{
	OnPartyMemberLeftEvent.Broadcast(MemberId, MemberStatus);
}

void UNexusPartyListenerComponent::OnPartyDisbanded(ENexusPartyResult PartyResult)
{
	OnPartyDisbandedEvent.Broadcast(PartyResult);
}

void UNexusPartyListenerComponent::OnPartyInviteReceived(const FNexusPendingPartyInvite& Invite)
{
	OnPartyInviteReceivedEvent.Broadcast(Invite);
}

void UNexusPartyListenerComponent::OnPartyGameSessionReady(const FNexusSearchResult& GameSession)
{
	OnPartyGameSessionReadyEvent.Broadcast(GameSession);
}
