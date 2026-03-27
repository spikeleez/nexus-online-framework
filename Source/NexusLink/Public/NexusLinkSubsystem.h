// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NexusLinkTypes.h"
#include "NexusLinkSubsystem.generated.h"

class UNexusLinkSessionManager;
class UNexusLinkFriendManager;

/**
 * @class UNexusLinkSubsystem
 * 
 * Core subsystem for the NexusLink plugin.
 * Automatically created when the GameInstance initializes.
 * Owns and provides access to the session and friend managers.
 *
 * Access from Blueprint: Get Game Instance -> Get Subsystem (NexusLink)
 * Access from C++: UGameInstance::GetSubsystem<UNexusLinkSubsystem>()
 * Access from anywhere: UNexusLinkSubsystem::Get(WorldContextObject)
 */
UCLASS(BlueprintType)
class NEXUSLINK_API UNexusLinkSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UNexusLinkSubsystem();

	//~Begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~End of USubsystem interface
	
	/** @return The session manager responsible for create/find/join/destroy operations. */
	UFUNCTION(BlueprintPure, Category = "NexusLink", meta = (DisplayName = "Get Session Manager"))
	UNexusLinkSessionManager* GetSessionManager() const { return SessionManager; }

	/** @return The friend manager responsible for friends list and invites. */
	UFUNCTION(BlueprintPure, Category = "NexusLink", meta = (DisplayName = "Get Friend Manager"))
	UNexusLinkFriendManager* GetFriendManager() const { return FriendManager; }

	/**
	 * Get the NexusLink subsystem from any world context object.
	 * This is the recommended way to access NexusLink from anywhere.
	 *
	 * @param WorldContextObject Any UObject with a valid world.
	 * @return The NexusLink subsystem, or nullptr if unavailable.
	 */
	UFUNCTION(BlueprintPure, Category = "NexusLink", meta = (WorldContext = "WorldContextObject", DisplayName = "Get NexusLink Subsystem"))
	static UNexusLinkSubsystem* Get(const UObject* WorldContextObject);

	/** @return Whether any online subsystem is currently available. */
	UFUNCTION(BlueprintPure, Category = "NexusLink")
	static bool IsOnlineSubsystemAvailable();

	/** @return The name of the currently active online subsystem (e.g. "Steam", "EOS", "NULL"). */
	UFUNCTION(BlueprintPure, Category = "NexusLink")
	static FString GetOnlineSubsystemName();

protected:
	/** Session management instance. Created during Initialize. */
	UPROPERTY()
	TObjectPtr<UNexusLinkSessionManager> SessionManager;

	/** Friend management instance. Created during Initialize. */
	UPROPERTY()
	TObjectPtr<UNexusLinkFriendManager> FriendManager;
};
