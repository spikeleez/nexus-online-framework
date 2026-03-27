// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * @class FNexusLinkModule
 *  
 * Main module class for the NexusLink plugin.
 * Handles module startup/shutdown lifecycle.
 */
class FNexusLinkModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
