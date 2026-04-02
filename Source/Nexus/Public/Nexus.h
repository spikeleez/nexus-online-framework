// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * @class FNexus
 *  
 * Main module class for the Nexus plugin.
 * Handles module startup/shutdown lifecycle.
 */
class FNexus : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
