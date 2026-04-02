// Copyright (c) 2026 spikeleez. All rights reserved.

#include "Nexus.h"
#include "NexusLog.h"

#define LOCTEXT_NAMESPACE "FNexusModule"

void FNexus::StartupModule()
{
	NEXUS_LOG(LogNexus, Log, TEXT("Nexus module started."));
}

void FNexus::ShutdownModule()
{
	NEXUS_LOG(LogNexus, Log, TEXT("Nexus module shutdown."));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNexus, Nexus)