// Copyright (c) 2026 spikeleez. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemTypes.h"
#include "NexusLog.h"
#include "Online/CoreOnline.h"
#include "GameFramework/OnlineReplStructs.h"
#include "NexusOnlineTypes.generated.h"

struct FPartyReservation;
struct FPlayerReservation;

/**
 * Result of a blueprint library output.
 */
UENUM(BlueprintType)
enum class ENexusBlueprintLibraryOutputResult : uint8
{
	IsValid,
	NotValid
};

/**
 * Result of a session update attempt.
 */
UENUM(BlueprintType)
enum class ENexusUpdateSessionResult : uint8
{
	Success,
	Failure,
	NoOnlineSubsystem,
	NoSession
};

/**
 * Represents the lifecycle state of an online session.
 * Mirrors EOnlineSessionState::Type but exposed to Blueprint with Nexus naming.
 */
UENUM(BlueprintType)
enum class ENexusSessionState : uint8
{
	/** No active session. */
	NoSession,

	/** Session is being created asynchronously. */
	Creating,

	/** Session created, waiting in lobby (pre-game). */
	Pending,

	/** Session is starting (communicating with backend). */
	Starting,

	/** Session is in progress. Join-in-progress may be restricted. */
	InProgress,

	/** Session is ending (post-game). */
	Ending,

	/** Session has ended and stats are committed. */
	Ended,

	/** Session is being destroyed. */
	Destroying
};

/** @return String representation of session state for logging. */
inline const TCHAR* LexToString(const ENexusSessionState Value)
{
	switch (Value)
	{
	case ENexusSessionState::NoSession:		return TEXT("NoSession");
	case ENexusSessionState::Creating:		return TEXT("Creating");
	case ENexusSessionState::Pending:		return TEXT("Pending");
	case ENexusSessionState::Starting:		return TEXT("Starting");
	case ENexusSessionState::InProgress:	return TEXT("InProgress");
	case ENexusSessionState::Ending:		return TEXT("Ending");
	case ENexusSessionState::Ended:			return TEXT("Ended");
	case ENexusSessionState::Destroying:	return TEXT("Destroying");
	default:								return TEXT("Unknown");
	}
}

/**
 * Result of a session creation attempt.
 */
UENUM(BlueprintType)
enum class ENexusCreateSessionResult : uint8
{
	/** Session created successfully. */
	Success,

	/** Generic failure. */
	Failure,

	/** A session with this name already exists. Destroy it first. */
	AlreadyExists,

	/** Online subsystem is not available. */
	NoOnlineSubsystem
};

/** @return String representation. */
inline const TCHAR* LexToString(const ENexusCreateSessionResult Value)
{
	switch (Value)
	{
	case ENexusCreateSessionResult::Success:			return TEXT("Success");
	case ENexusCreateSessionResult::Failure:			return TEXT("Failure");
	case ENexusCreateSessionResult::AlreadyExists:		return TEXT("AlreadyExists");
	case ENexusCreateSessionResult::NoOnlineSubsystem:	return TEXT("NoOnlineSubsystem");
	default:											return TEXT("Unknown");
	}
}

/**
 * Result of a find sessions operation.
 */
UENUM(BlueprintType)
enum class ENexusFindSessionsResult : uint8
{
	/** Search completed and found at least one session. */
	Success,

	/** Search completed but returned zero results. */
	NoResults,

	/** Search failed internally. */
	Failure,

	/** Online subsystem is not available. */
	NoOnlineSubsystem
};

/** @return String representation. */
inline const TCHAR* LexToString(const ENexusFindSessionsResult Value)
{
	switch (Value)
	{
	case ENexusFindSessionsResult::Success:				return TEXT("Success");
	case ENexusFindSessionsResult::NoResults:			return TEXT("NoResults");
	case ENexusFindSessionsResult::Failure:				return TEXT("Failure");
	case ENexusFindSessionsResult::NoOnlineSubsystem:	return TEXT("NoOnlineSubsystem");
	default:											return TEXT("Unknown");
	}
}

/**
 * Result of a session join attempt.
 */
UENUM(BlueprintType)
enum class ENexusJoinSessionResult : uint8
{
	/** Successfully joined the session. */
	Success,

	/** Generic failure. */
	Failure,

	/** Session is full no open slots. */
	SessionFull,

	/** Session does not exist or was already destroyed. */
	SessionNotFound,

	/** Local player is already in a session with this name. */
	AlreadyInSession,

	/** Online subsystem is not available. */
	NoOnlineSubsystem
};

/** @return String representation. */
inline const TCHAR* LexToString(const ENexusJoinSessionResult Value)
{
	switch (Value)
	{
	case ENexusJoinSessionResult::Success:				return TEXT("Success");
	case ENexusJoinSessionResult::Failure:				return TEXT("Failure");
	case ENexusJoinSessionResult::SessionFull:			return TEXT("SessionFull");
	case ENexusJoinSessionResult::SessionNotFound:		return TEXT("SessionNotFound");
	case ENexusJoinSessionResult::AlreadyInSession:		return TEXT("AlreadyInSession");
	case ENexusJoinSessionResult::NoOnlineSubsystem:	return TEXT("NoOnlineSubsystem");
	default:											return TEXT("Unknown");
	}
}

/**
 * Result of a destroy session operation.
 */
UENUM(BlueprintType)
enum class ENexusDestroySessionResult : uint8
{
	/** Session destroyed successfully. */
	Success,

	/** Failed to destroy the session. */
	Failure,

	/** No active session to destroy. */
	NoSession
};

/** @return String representation. */
inline const TCHAR* LexToString(const ENexusDestroySessionResult Value)
{
	switch (Value)
	{
	case ENexusDestroySessionResult::Success:	return TEXT("Success");
	case ENexusDestroySessionResult::Failure:	return TEXT("Failure");
	case ENexusDestroySessionResult::NoSession:	return TEXT("NoSession");
	default:									return TEXT("Unknown");
	}
}

/**
 * Online presence status of a friend.
 */
UENUM(BlueprintType)
enum class ENexusFriendPresence : uint8
{
	/** Friend is offline. */
	Offline,

	/** Friend is online but not in our game. */
	Online,

	/** Friend is online and playing our game. */
	InGame,

	/** Friend is set to away/idle. */
	Away
};

/** @return String representation. */
inline const TCHAR* LexToString(const ENexusFriendPresence Value)
{
	switch (Value)
	{
	case ENexusFriendPresence::Offline:		return TEXT("Offline");
	case ENexusFriendPresence::Online:		return TEXT("Online");
	case ENexusFriendPresence::InGame:		return TEXT("InGame");
	case ENexusFriendPresence::Away:		return TEXT("Away");
	default:								return TEXT("Unknown");
	}
}

/**
 * Blueprint-friendly wrapper around EOnlineComparisonOp.
 * Used when defining query settings for session searches.
 */
UENUM(BlueprintType)
enum class ENexuQueryComparisonOp : uint8
{
	Equals,
	NotEquals,
	GreaterThan,
	GreaterThanEquals,
	LessThan,
	LessThanEquals
};

/**
 * Maps to EPartyReservationResult from the engine.
 * We mirror it here to avoid exposing engine internals to Blueprint consumers.
 */
UENUM(BlueprintType)
enum class ENexusReservationResult : uint8
{
	/** Reservation accepted. Client should now join the game session. */
	Success,

	/** Session is at capacity no slots available. */
	SessionFull,

	/** Party is too large for the remaining open slots. */
	PartyTooLarge,

	/** A reservation already exists for this party leader. */
	ReservationDuplicate,

	/** The party leader ID is not recognized or is invalid. */
	ReservationNotFound,

	/** Reservation request contained invalid data. */
	BadSessionId,

	/** Duplicate players detected within the same reservation request. */
	DuplicateReservation,

	/** Reservation was accepted but the team could not be matched (crossplay/platform mismatch). */
	IncorrectPlayerCount,

	/** Unknown error. */
	GeneralError
};

/** @return String representation for logging. */
FORCEINLINE FString LexToString(ENexusReservationResult Result)
{
	switch (Result)
	{
	case ENexusReservationResult::Success:               return TEXT("Success");
	case ENexusReservationResult::SessionFull:           return TEXT("SessionFull");
	case ENexusReservationResult::PartyTooLarge:         return TEXT("PartyTooLarge");
	case ENexusReservationResult::ReservationDuplicate:  return TEXT("ReservationDuplicate");
	case ENexusReservationResult::ReservationNotFound:   return TEXT("ReservationNotFound");
	case ENexusReservationResult::BadSessionId:          return TEXT("BadSessionId");
	case ENexusReservationResult::DuplicateReservation:  return TEXT("DuplicateReservation");
	case ENexusReservationResult::IncorrectPlayerCount:  return TEXT("IncorrectPlayerCount");
	default:											 return TEXT("GeneralError");
	}
}

/**
 * Represents a single player inside a party reservation request.
 *
 * Platform is optional and used for crossplay filtering.
 * ValidationStr is used by custom anti-cheat hooks leave empty if unused.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexusPartyMember
{
	GENERATED_BODY()

	/** The player's unique online ID. */
	UPROPERTY(BlueprintReadWrite, Category = "Nexus|Party")
	FUniqueNetIdRepl UniqueId;

	/**
	 * The platform the player is on (e.g. "Steam", "EOS", "XboxOne").
	 * Used for crossplay filtering. Leave empty to skip platform checks.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Nexus|Party")
	FString Platform;

	/**
	 * Optional anti-cheat validation string.
	 * Checked server-side if bValidateReservations is enabled in the ReservationManager.
	 * Leave empty if you don't use custom token validation.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Nexus|Party")
	FString ValidationStr;

	/** Whether this player allows cross-platform play. */
	UPROPERTY(BlueprintReadWrite, Category = "Nexus|Party")
	bool bAllowCrossPlay = true;

	FNexusPartyMember() = default;

	explicit FNexusPartyMember(const FUniqueNetIdRepl& InId, const FString& InPlatform = FString())
		: UniqueId(InId)
		, Platform(InPlatform)
	{
	}

	bool IsValid() const { return UniqueId.IsValid(); }

	/** Convert to engine FPlayerReservation for use with APartyBeaconClient/Host. */
	FPlayerReservation ToNative() const;
};

/**
 * A party reservation request: one leader + zero or more additional members.
 *
 * Solo players set PartyLeader and leave PartyMembers empty.
 * The leader MUST be a valid unique ID.
 *
 * TeamNum: use -1 (auto) in almost all cases. Manual team assignment is only
 * needed for tournament/bracket scenarios.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexusPartyReservation
{
	GENERATED_BODY()

	/** The party leader. Must be valid. */
	UPROPERTY(BlueprintReadWrite, Category = "Nexus|Party")
	FNexusPartyMember PartyLeader;

	/**
	 * Additional party members, not including the leader.
	 * For a solo reservation, leave this empty.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Nexus|Party")
	TArray<FNexusPartyMember> PartyMembers;

	/**
	 * Requested team number. -1 = auto-assign.
	 * Only set manually if you have team-locked matchmaking.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Nexus|Party")
	int32 TeamNum = INDEX_NONE;

	FNexusPartyReservation() = default;

	/** @return Total party size including the leader. */
	int32 GetPartySize() const { return 1 + PartyMembers.Num(); }

	/** @return Whether the reservation has a valid leader. */
	bool IsValid() const { return PartyLeader.IsValid(); }

	/** Convert to engine FPartyReservation. */
	FPartyReservation ToNative() const;
};

/**
 * Summary of a single accepted reservation, used in server-side delegates.
 * Read-only produced by UNexusLinkReservationManager on incoming requests.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexusReservationSummary
{
	GENERATED_BODY()

	/** The party leader's unique ID. Use as the canonical party identifier. */
	UPROPERTY(BlueprintReadOnly, Category = "NexusLink|Party")
	FUniqueNetIdRepl PartyLeaderId;

	/** Total slots this reservation consumed. */
	UPROPERTY(BlueprintReadOnly, Category = "NexusLink|Party")
	int32 PartySize = 0;

	/** Assigned team index. -1 if not team-based. */
	UPROPERTY(BlueprintReadOnly, Category = "NexusLink|Party")
	int32 TeamNum = -1;

	bool IsValid() const { return PartyLeaderId.IsValid() && PartySize > 0; }
};

/**
 * A single session setting (key-value pair) stored on a session.
 * Uses FVariantData internally for type-safe storage across different value types.
 * Multiple typed constructors provided for C++ convenience.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexuSessionSetting
{
	GENERATED_BODY()

	/** Setting key. Must be unique within the session. */
	FName Key;

	/** The actual typed value. Not directly Blueprint-visible; use the typed constructors or Statics helpers. */
	FVariantData Data;

	/** How the setting is advertised with the online backend. */
	EOnlineDataAdvertisementType::Type AdvertisementType;

	/** Default constructor. */
	FNexuSessionSetting();

	/** Constructor from FVariantData. */
	FNexuSessionSetting(const FName InKey, const FVariantData& InData, const EOnlineDataAdvertisementType::Type InAdvType);

	/** Templated constructor for arbitrary value types (int32, FString, float, etc). */
	template<typename ValueType>
	FNexuSessionSetting(const FName InKey, const ValueType InValue, const EOnlineDataAdvertisementType::Type InAdvType)
		: Key(InKey)
		, Data(FVariantData())
		, AdvertisementType(InAdvType)
	{
		Data.SetValue(InValue);
	}

	/** @return Whether this setting is valid (has a key and data). */
	bool IsValid() const
	{
		return Key != NAME_None && Data.GetType() != EOnlineKeyValuePairDataType::Empty;
	}
};

/**
 * A single query setting used when searching for sessions.
 * Pairs a key-value with a comparison operator for server-side or local filtering.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexusQuerySetting
{
	GENERATED_BODY()

	/** Setting key to compare against. */
	FName Key;

	/** The value to compare with. */
	FVariantData Data;

	/** How to compare the values. */
	EOnlineComparisonOp::Type ComparisonOp;

	/** Default constructor. */
	FNexusQuerySetting();

	/** Constructor from FVariantData. */
	FNexusQuerySetting(const FName InKey, const FVariantData& InData, const EOnlineComparisonOp::Type InCompOp);

	/** Templated constructor for arbitrary value types. */
	template<typename ValueType>
	FNexusQuerySetting(const FName InKey, const ValueType InValue, const EOnlineComparisonOp::Type InCompOp)
		: Key(InKey)
		, Data(FVariantData())
		, ComparisonOp(InCompOp)
	{
		Data.SetValue(InValue);
	}

	/** @return Whether this query setting is valid. */
	bool IsValid() const
	{
		// We don't accept Near, In, NotIn comparison types they require special handling.
		const bool bComparisonOpValid = ComparisonOp < EOnlineComparisonOp::Near;
		return Key != NAME_None && Data.GetType() != EOnlineKeyValuePairDataType::Empty && bComparisonOpValid;
	}

	/**
	 * Compare this query setting against a session setting value.
	 * Assumes the session setting has the same value type as the query.
	 * Used when auto-filtering search results locally.
	 *
	 * @param SessionSetting The session setting to compare against. Can be nullptr.
	 * @return True if the comparison passes. False if nullptr or mismatch.
	 */
	template<typename ValueType>
	bool CompareAgainst(const FOnlineSessionSetting* SessionSetting) const
	{
		if (!SessionSetting)
		{
			NEXUS_LOG(LogNexus, Error, TEXT("CompareAgainst called with nullptr!"));
			return false;
		}

		ValueType QueryValue;
		Data.GetValue(QueryValue);

		ValueType SessionValue;
		SessionSetting->Data.GetValue(SessionValue);

		switch (ComparisonOp)
		{
		case EOnlineComparisonOp::Equals:				return SessionValue == QueryValue;
		case EOnlineComparisonOp::NotEquals:			return SessionValue != QueryValue;
		case EOnlineComparisonOp::GreaterThan:			return SessionValue > QueryValue;
		case EOnlineComparisonOp::GreaterThanEquals:	return SessionValue >= QueryValue;
		case EOnlineComparisonOp::LessThan:				return SessionValue < QueryValue;
		case EOnlineComparisonOp::LessThanEquals:		return SessionValue <= QueryValue;
		default:										return false;
		}
	}
};

/**
 * Blueprint-friendly wrapper around FOnlineSessionSettings.
 * Reads and exposes commonly used fields from the native session settings.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexusSessionSettings
{
	GENERATED_BODY()

	/** The underlying native session settings. Not exposed to Blueprint. */
	FOnlineSessionSettings SessionSettings;

	/** Server/Lobby display name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	FString ServerName;

	/** Map name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	FString MapName;

	/** Game mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	FString GameMode;

	/** Max session capacity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	int32 MaxNumPlayers;

	/** Whether the session is advertised. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	bool bShouldAdvertise;

	/** Whether the session is hidden from normal searches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	bool bHidden;

	/** Whether players can join after the game starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	bool bAllowJoinInProgress;

	/** Whether this is a LAN match. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	bool bIsLanMatch;

	/** Whether the session uses presence (lobbies). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	bool bUsesPresence;

	/** Whether invites are allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	bool bAllowInvites;

	/** Whether joining via presence is allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus")
	bool bAllowJoinViaPresence;

	/** Default constructor. */
	FNexusSessionSettings();

	/** Constructor from native FOnlineSessionSettings. Extracts all known custom keys. */
	FNexusSessionSettings(const FOnlineSessionSettings& InSessionSettings);

	/**
	 * Get a custom session setting by key.
	 *
	 * @param Key Setting key.
	 * @param OutValue The output value.
	 * @return True if the setting was found and extracted.
	 */
	template<typename ValueType>
	bool GetSessionSetting(const FName Key, ValueType& OutValue) const
	{
		return SessionSettings.Get(Key, OutValue);
	}
};

/**
 * Parameters used when creating (hosting) a new session.
 * Passed to CreateSession to define the session's properties.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexuHostParams
{
	GENERATED_BODY()

	/**
	 * The map to load via ServerTravel after session creation.
	 * Empty string = do not travel (stay on current map).
	 * Example: /Game/Maps/Dungeon_01
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	FString StartingLevel;

	/** Display name of the server/lobby. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	FString ServerName;

	/** Map name metadata stored on the session for display/filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	FString MapName;

	/** Game mode metadata stored on the session for display/filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	FString GameMode;

	/** Maximum number of players allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MaxNumPlayers;

	/** Whether the session should be publicly advertised. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	bool bShouldAdvertise;

	/** Whether the session is hidden from normal searches (invite-only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	bool bHidden;

	/** Whether players can join after the game has started. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	bool bAllowJoinInProgress;

	/** Whether this is a LAN match. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	bool bIsLanMatch;

	/** Whether the session uses presence (Steam lobbies, EOS lobbies). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	bool bUsesPresence;

	/** Whether invites to this session are allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	bool bAllowInvites;

	/** Whether players can join via presence (friend list join). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Host")
	bool bAllowJoinViaPresence;

	/** Additional custom session settings applied on top of the defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Nexus|Host")
	TArray<FNexuSessionSetting> ExtraSessionSettings;

	/**
	 * Optional: override the FOnlineSessionSettings entirely.
	 * When set, the individual fields above are ignored and this is used directly.
	 * Not exposed to Blueprint C++ power-user feature.
	 */
	TOptional<FOnlineSessionSettings> SessionSettingsOverride;

	/** Default constructor with sensible defaults for a co-op game. */
	FNexuHostParams();

	/**
	 * Build FOnlineSessionSettings from these host params.
	 * If SessionSettingsOverride is set, copies that instead.
	 *
	 * @param OutSettings The populated session settings.
	 */
	void ToOnlineSessionSettings(FOnlineSessionSettings& OutSettings) const;

	/** @return Whether these host parameters pass validation. */
	bool IsValid(const bool bLogErrors = true) const;

	/** @return Whether session settings are overridden. */
	FORCEINLINE bool HasSessionSettingsOverride() const
	{
		return SessionSettingsOverride.IsSet();
	}
};

/**
 * Parameters used when searching for sessions.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexusSearchParams
{
	GENERATED_BODY()

	/** Maximum number of results to return. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Search", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxSearchResults;

	/** Whether to search for LAN sessions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Search")
	bool bIsLanQuery;

	/** Whether to search presence-based sessions (lobbies). Recommended for Steam/EOS. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Search")
	bool bSearchPresence;

	/** Extra query settings for server-side or local filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Nexus|Search")
	TArray<FNexusQuerySetting> ExtraQuerySettings;

	/** List of session unique IDs to exclude from results. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Nexus|Search")
	TArray<FUniqueNetIdRepl> IgnoredSessions;

	/** Default constructor. */
	FNexusSearchParams();

	/** @return Whether these search parameters pass validation. */
	bool IsValid(const bool bLogErrors = true) const;
};

/**
 * A single search result representing a found online session.
 * Wraps FOnlineSessionSearchResult with Blueprint-accessible helpers and sort predicates.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexusSearchResult
{
	GENERATED_BODY()

	/** The underlying online search result. Not exposed to Blueprint. */
	FOnlineSessionSearchResult OnlineResult;

	/** Default constructor. */
	FNexusSearchResult();

	/** Constructor from a native online search result. */
	FNexusSearchResult(const FOnlineSessionSearchResult& InResult);

	/** @return Whether this search result contains valid data. */
	bool IsValid() const;

	/** @return The session type name (usually NAME_GameSession). */
	FName GetSessionType() const;

	/** @return The unique ID of the session itself. */
	FUniqueNetIdRepl GetSessionUniqueId() const;

	/** @return The unique ID of the session owner. */
	FUniqueNetIdRepl GetOwnerUniqueId() const;

	/** @return The display name of the session owner (truncated to 20 chars for safety). */
	FString GetOwnerUsername() const;

	/** @return Current number of players in the session. */
	int32 GetNumPlayers() const;

	/** @return Number of open (available) public slots. */
	int32 GetNumOpenSlots() const;

	/** @return Max number of public connections. */
	int32 GetMaxPlayers() const;

	/** @return Ping to the session in milliseconds. -1 if unknown. */
	int32 GetPing() const;

	/** @return Session settings wrapped in our Blueprint-friendly struct. */
	FNexusSessionSettings GetSessionSettings() const;

	/**
	 * Returns the native engine search result.
	 * Required for UNexusLinkBeaconManager::ConnectPingBeaconToSession
	 * and any direct OSS calls that need the raw FOnlineSessionSearchResult.
	 */
	const FOnlineSessionSearchResult& GetNativeSearchResult() const { return OnlineResult; }

	/**
	 * Get a specific custom session setting value.
	 *
	 * @param Key Setting key.
	 * @param OutValue The output value.
	 * @return True if the setting was found.
	 */
	template<typename ValueType>
	bool GetSessionSetting(const FName Key, ValueType& OutValue) const
	{
		return OnlineResult.Session.SessionSettings.Get(Key, OutValue);
	}

	// -- Sort Predicates (for use with TArray::Sort) --

	/** Sort by ping ascending (lowest ping first). */
	static bool ComparePing(const FNexusSearchResult& A, const FNexusSearchResult& B)
	{
		return A.GetPing() < B.GetPing();
	}

	/** Sort by player count descending (fullest sessions first). */
	static bool ComparePlayerCountDesc(const FNexusSearchResult& A, const FNexusSearchResult& B)
	{
		return A.GetNumPlayers() > B.GetNumPlayers();
	}

	/** Sort by open slots ascending (least available first). */
	static bool CompareOpenSlotsAsc(const FNexusSearchResult& A, const FNexusSearchResult& B)
	{
		return A.GetNumOpenSlots() < B.GetNumOpenSlots();
	}
};

/**
 * Blueprint-friendly wrapper around FNamedOnlineSession.
 * Represents a session that the local player is currently part of.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexusNamedSession
{
	GENERATED_BODY()

	/** Pointer to the native named session in the online subsystem. Not owned by us. */
	const FNamedOnlineSession* OnlineSession;

	/** Default constructor. */
	FNexusNamedSession()
		: OnlineSession(nullptr)
	{}

	/** Constructor from a native named session. */
	FNexusNamedSession(const FNamedOnlineSession* InSession)
		: OnlineSession(InSession)
	{}

	/** @return Whether the named session is valid. */
	bool IsValid() const { return OnlineSession != nullptr; }

	/** @return The session name (e.g. NAME_GameSession). */
	FName GetSessionName() const
	{
		return IsValid() ? OnlineSession->SessionName : NAME_None;
	}

	/** @return Current state of the session mapped to our enum. */
	ENexusSessionState GetSessionState() const
	{
		return IsValid() ? static_cast<ENexusSessionState>(OnlineSession->SessionState) : ENexusSessionState::NoSession;
	}

	/** @return Session settings wrapped in our struct. */
	FNexusSessionSettings GetSessionSettings() const
	{
		return IsValid() ? FNexusSessionSettings(OnlineSession->SessionSettings) : FNexusSessionSettings();
	}

	/** @return The unique ID of the session. */
	FUniqueNetIdRepl GetSessionUniqueId() const
	{
		if (IsValid() && OnlineSession->SessionInfo.IsValid())
		{
			return FUniqueNetIdRepl(OnlineSession->SessionInfo->GetSessionId());
		}
		return FUniqueNetIdRepl();
	}

	/** @return The unique ID of the session owner. */
	FUniqueNetIdRepl GetOwnerUniqueId() const
	{
		return IsValid() ? FUniqueNetIdRepl(OnlineSession->OwningUserId) : FUniqueNetIdRepl();
	}

	/** @return The display name of the session owner. */
	FString GetOwnerUsername() const
	{
		return IsValid() ? OnlineSession->OwningUserName.Left(20) : FString();
	}

	/** @return Current number of players in the session. */
	int32 GetNumPlayers() const
	{
		if (!IsValid()) return 0;
		return OnlineSession->SessionSettings.NumPublicConnections - OnlineSession->NumOpenPublicConnections;
	}
};

/**
 * Blueprint-friendly wrapper around a native FOnlineFriend.
 * Exposes display name, unique ID, and presence information.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexusOnlineFriend
{
	GENERATED_BODY()

	/** Unique net ID of the friend. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Nexus")
	FUniqueNetIdRepl UserId;

	/** Display name of the friend. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Nexus")
	FString DisplayName;

	/** Current presence status. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Nexus")
	ENexusFriendPresence Presence;

	/** Whether the friend is online (convenience, derived from Presence). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Nexus")
	bool bIsOnline;

	/** Whether the friend is playing our game (convenience, derived from Presence). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Nexus")
	bool bIsInGame;

	/** Default constructor. */
	FNexusOnlineFriend();

	/** Constructor from a native FOnlineFriend. Extracts presence data automatically. */
	FNexusOnlineFriend(const FOnlineFriend& NativeFriend);

	/** @return Whether this friend data is valid. */
	bool IsValid() const { return UserId.IsValid(); }
};

/**
 * Represents a session invite received by the local player.
 * Stored until the player accepts, declines, or the invite expires.
 */
USTRUCT(BlueprintType)
struct NEXUS_API FNexusPendingInvite
{
	GENERATED_BODY()

	/** Unique ID of the player who sent the invite. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Nexus")
	FUniqueNetIdRepl FromId;

	/** The session that the invite points to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Nexus")
	FNexusSearchResult Session;

	/** Real time (seconds since app start) when the invite was received. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Nexus")
	float TimeReceived;

	/** Default constructor. */
	FNexusPendingInvite()
		: FromId(FUniqueNetIdRepl())
		, Session(FNexusSearchResult())
		, TimeReceived(0.0f)
	{}

	/** @return Whether the invite is valid. */
	bool IsValid() const { return FromId.IsValid() && Session.IsValid(); }
};

/** Fired when a session is created. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusOnSessionCreatedSignature, ENexusCreateSessionResult, Result);

/** Fired when sessions are found. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNexusOnSessionsFoundSignature, ENexusFindSessionsResult, Result, const TArray<FNexusSearchResult>&, Results);

/** Fired when a session is joined. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusOnSessionJoinedSignature, ENexusJoinSessionResult, Result);

/** Fired when a session is destroyed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusOnSessionDestroyedSignature, ENexusDestroySessionResult, Result);

/** Fired when the session state changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNexusOnSessionStateChangedSignature, FName, SessionName, ENexusSessionState, NewState);

/** Fired when the friends list is read. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNexusOnFriendsListReadySignature, bool, bWasSuccessful, const TArray<FNexusOnlineFriend>&, Friends);

/** Fired when a session invite is received from another player. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusOnSessionInviteReceivedSignature, const FNexusPendingInvite&, Invite);

/** Fired when the local user accepts a session invite (e.g. from Steam Overlay). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusOnSessionInviteAcceptedSignature, const FNexusSearchResult&, InviteResult);

/** Fired when a session is updated. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusOnSessionUpdatedSignature, const ENexusUpdateSessionResult, Result);

DECLARE_MULTICAST_DELEGATE_OneParam(FNexusNativeOnSessionCreatedSignature, ENexusCreateSessionResult /*Result*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FNexusNativeOnSessionsFoundSignature, ENexusFindSessionsResult /*Result*/, const TArray<FNexusSearchResult>& /*Results*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FNexusNativeOnSessionJoinedSignature, ENexusJoinSessionResult /*Result*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FNexusNativeOnSessionDestroyedSignature, ENexusDestroySessionResult /*Result*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FNexusNativeOnSessionUpdatedSignature, const ENexusUpdateSessionResult);
