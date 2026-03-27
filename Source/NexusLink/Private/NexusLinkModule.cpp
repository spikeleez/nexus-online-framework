// Copyright (c) 2026 spikeleez. All rights reserved.

#include "NexusLinkModule.h"
#include "NexusLog.h"

#define LOCTEXT_NAMESPACE "FNexusLinkModule"

void FNexusLinkModule::StartupModule()
{
	NEXUS_LOG(LogNexusLink, Log, TEXT("NexusLink module started."));
}

void FNexusLinkModule::ShutdownModule()
{
	NEXUS_LOG(LogNexusLink, Log, TEXT("NexusLink module shutdown."));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNexusLinkModule, NexusLink)