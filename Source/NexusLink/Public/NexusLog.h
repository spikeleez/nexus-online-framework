// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "Logging/LogMacros.h"

/** Nexus log category. */
NEXUSLINK_API DECLARE_LOG_CATEGORY_EXTERN(LogNexusLink, Log, All);

/** Whether the custom log macros should colorize the output text messages. */
#define HIGHLIGHT_LOGS 1

#define NEXUS_PRINT_FILE (FString(FPaths::GetCleanFilename(TEXT(__FILE__))))
#define NEXUS_PRINT_FUNC (FString(__FUNCTION__))
#define NEXUS_PRINT_LINE (FString::FromInt(__LINE__))
#define NEXUS_LOGS_LINE (NEXUS_PRINT_FUNC + TEXT(" [") + NEXUS_PRINT_FILE + TEXT(":") + NEXUS_PRINT_LINE + TEXT("]"))

/** Nexus custom default log */
#define NEXUS_LOG(CategoryName, Verbosity, Format, ...) UE_LOG(CategoryName, Verbosity, TEXT("%s %s"), *NEXUS_LOGS_LINE, *FString::Printf(Format, ##__VA_ARGS__))

/** A custom conditional log macro that can colorize the output message. */
#define NEXUS_CLOG(Conditional, Category, Verbosity, Format, ...) \
{ \
	bool bColorizeOutput = HIGHLIGHT_LOGS && ELogVerbosity::Verbosity >= ELogVerbosity::Display; \
	UE_CLOG(Conditional, Category, Verbosity, Format, ##__VA_ARGS__); \
}

/** A custom log macro that can colorize the output message. */
#define NEXUS_LOG_COLOR(Category, Verbosity, Color, Format, ...) \
{ \
	bool bColorizeOutput = HIGHLIGHT_LOGS && PLATFORM_SUPPORTS_COLORIZED_OUTPUT_DEVICE && ELogVerbosity::Verbosity >= ELogVerbosity::Display; \
	if (bColorizeOutput) SET_WARN_COLOR(Color); \
	UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__); \
	if (bColorizeOutput) CLEAR_WARN_COLOR(); \
}

/** A custom conditional log macro that can colorize the output message. */
#define NEXUS_CLOG_COLOR(Conditional, Category, Verbosity, Color, Format, ...) \
{ \
	bool bColorizeOutput = HIGHLIGHT_LOGS && PLATFORM_SUPPORTS_COLORIZED_OUTPUT_DEVICE && ELogVerbosity::Verbosity >= ELogVerbosity::Display; \
	if (bColorizeOutput) SET_WARN_COLOR(Color); \
	UE_CLOG(Conditional, Category, Verbosity, Format, ##__VA_ARGS__); \
	if (bColorizeOutput) CLEAR_WARN_COLOR(); \
}

/** Setting describing the session type. This will tell us how to handle the session (value is FString) */
#define NEXUS_SESSIONTYPE FName(TEXT("SESSIONTYPE"))
/** Setting describing the session owner's unique id (value is FString) */
#define NEXUS_OWNERID FName(TEXT("OWNERID"))
/** Setting describing whether the session uses the reservation system (value is int32 because the Steam Subsystem doesn't support bool queries) */
#define NEXUS_USERESERVATIONS FName(TEXT("USERESERVATIONS"))
/** Setting describing whether the session is hidden or not (value is int32 because the Steam Subsystem doesn't support bool queries) */
#define NEXUS_HIDDEN FName(TEXT("HIDDEN"))
/** Setting describing the session's display name (value is FString) */
#define NEXUS_SERVERNAME FName(TEXT("SERVERNAME"))
/** Setting describing the session's map display name (value is FString) */
#define NEXUS_MAPNAME FName(TEXT("MAPNAME"))
/** Setting describing the session's gamemode display name (value is FString) */
#define NEXUS_GAMEMODE FName(TEXT("GAMEMODE"))
/** Setting describing which playlist the session belongs to (value is FString) */
#define NEXUS_PLAYLIST FName(TEXT("PLAYLIST"))
/** Setting describing the session's skill level (value is int32) */
#define NEXUS_SESSIONELO FName(TEXT("SESSIONELO"))
/** Second key for session elo because query settings can only compare against one session setting (value is int32) */
#define NEXUS_SESSIONELO2 FName(TEXT("SESSIONELO2"))
/** Setting describing which players are not allowed to join the session (value is FString of the form "uniqueid1;uniqueid2;uniqueid3") */
#define NEXUS_BANNEDPLAYERS FName(TEXT("BANNEDPLAYERS"))
/** Setting describing which level should be opened by the host once the session is created (value is FString) */
#define NEXUS_STARTINGLEVEL FName(TEXT("STARTINGLEVEL"))
/** Setting describing the reconnect identifier. Reconnecting clients use this to confirm that they are reconnecting the proper session (value is FString) */
#define NEXUS_RECONNECTID FName(TEXT("RECONNECTID"))