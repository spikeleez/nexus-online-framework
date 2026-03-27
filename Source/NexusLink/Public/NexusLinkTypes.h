#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemTypes.h"
#include "NexusLog.h"
#include "NexusLinkTypes.generated.h"

/**
 * Represents the lifecycle state of an online session.
 * Mirrors EOnlineSessionState::Type but exposed to Blueprint with NexusLink naming.
 */
UENUM(BlueprintType)
enum class ENexusLinkSessionState : uint8
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
inline const TCHAR* LexToString(const ENexusLinkSessionState Value)
{
	switch (Value)
	{
	case ENexusLinkSessionState::NoSession:		return TEXT("NoSession");
	case ENexusLinkSessionState::Creating:		return TEXT("Creating");
	case ENexusLinkSessionState::Pending:		return TEXT("Pending");
	case ENexusLinkSessionState::Starting:		return TEXT("Starting");
	case ENexusLinkSessionState::InProgress:	return TEXT("InProgress");
	case ENexusLinkSessionState::Ending:		return TEXT("Ending");
	case ENexusLinkSessionState::Ended:			return TEXT("Ended");
	case ENexusLinkSessionState::Destroying:	return TEXT("Destroying");
	default:									return TEXT("Unknown");
	}
}

/**
 * Result of a session creation attempt.
 */
UENUM(BlueprintType)
enum class ENexusLinkCreateSessionResult : uint8
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
inline const TCHAR* LexToString(const ENexusLinkCreateSessionResult Value)
{
	switch (Value)
	{
	case ENexusLinkCreateSessionResult::Success:			return TEXT("Success");
	case ENexusLinkCreateSessionResult::Failure:			return TEXT("Failure");
	case ENexusLinkCreateSessionResult::AlreadyExists:		return TEXT("AlreadyExists");
	case ENexusLinkCreateSessionResult::NoOnlineSubsystem:	return TEXT("NoOnlineSubsystem");
	default:												return TEXT("Unknown");
	}
}

/**
 * Result of a find sessions operation.
 */
UENUM(BlueprintType)
enum class ENexusLinkFindSessionsResult : uint8
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
inline const TCHAR* LexToString(const ENexusLinkFindSessionsResult Value)
{
	switch (Value)
	{
	case ENexusLinkFindSessionsResult::Success:				return TEXT("Success");
	case ENexusLinkFindSessionsResult::NoResults:			return TEXT("NoResults");
	case ENexusLinkFindSessionsResult::Failure:				return TEXT("Failure");
	case ENexusLinkFindSessionsResult::NoOnlineSubsystem:	return TEXT("NoOnlineSubsystem");
	default:												return TEXT("Unknown");
	}
}

/**
 * Result of a session join attempt.
 */
UENUM(BlueprintType)
enum class ENexusLinkJoinSessionResult : uint8
{
	/** Successfully joined the session. */
	Success,

	/** Generic failure. */
	Failure,

	/** Session is full — no open slots. */
	SessionFull,

	/** Session does not exist or was already destroyed. */
	SessionNotFound,

	/** Local player is already in a session with this name. */
	AlreadyInSession,

	/** Online subsystem is not available. */
	NoOnlineSubsystem
};

/** @return String representation. */
inline const TCHAR* LexToString(const ENexusLinkJoinSessionResult Value)
{
	switch (Value)
	{
	case ENexusLinkJoinSessionResult::Success:				return TEXT("Success");
	case ENexusLinkJoinSessionResult::Failure:				return TEXT("Failure");
	case ENexusLinkJoinSessionResult::SessionFull:			return TEXT("SessionFull");
	case ENexusLinkJoinSessionResult::SessionNotFound:		return TEXT("SessionNotFound");
	case ENexusLinkJoinSessionResult::AlreadyInSession:		return TEXT("AlreadyInSession");
	case ENexusLinkJoinSessionResult::NoOnlineSubsystem:	return TEXT("NoOnlineSubsystem");
	default:												return TEXT("Unknown");
	}
}

/**
 * Result of a destroy session operation.
 */
UENUM(BlueprintType)
enum class ENexusLinkDestroySessionResult : uint8
{
	/** Session destroyed successfully. */
	Success,

	/** Failed to destroy the session. */
	Failure,

	/** No active session to destroy. */
	NoSession
};

/** @return String representation. */
inline const TCHAR* LexToString(const ENexusLinkDestroySessionResult Value)
{
	switch (Value)
	{
	case ENexusLinkDestroySessionResult::Success:	return TEXT("Success");
	case ENexusLinkDestroySessionResult::Failure:	return TEXT("Failure");
	case ENexusLinkDestroySessionResult::NoSession:	return TEXT("NoSession");
	default:										return TEXT("Unknown");
	}
}

/**
 * Online presence status of a friend.
 */
UENUM(BlueprintType)
enum class ENexusLinkFriendPresence : uint8
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
inline const TCHAR* LexToString(const ENexusLinkFriendPresence Value)
{
	switch (Value)
	{
	case ENexusLinkFriendPresence::Offline:		return TEXT("Offline");
	case ENexusLinkFriendPresence::Online:		return TEXT("Online");
	case ENexusLinkFriendPresence::InGame:		return TEXT("InGame");
	case ENexusLinkFriendPresence::Away:		return TEXT("Away");
	default:									return TEXT("Unknown");
	}
}

/**
 * Blueprint-friendly wrapper around EOnlineComparisonOp.
 * Used when defining query settings for session searches.
 */
UENUM(BlueprintType)
enum class ENexusLinkQueryComparisonOp : uint8
{
	Equals,
	NotEquals,
	GreaterThan,
	GreaterThanEquals,
	LessThan,
	LessThanEquals
};

/**
 * A single session setting (key-value pair) stored on a session.
 * Uses FVariantData internally for type-safe storage across different value types.
 * Multiple typed constructors provided for C++ convenience.
 */
USTRUCT(BlueprintType)
struct NEXUSLINK_API FNexusLinkSessionSetting
{
	GENERATED_BODY()

	/** Setting key. Must be unique within the session. */
	FName Key;

	/** The actual typed value. Not directly Blueprint-visible; use the typed constructors or Statics helpers. */
	FVariantData Data;

	/** How the setting is advertised with the online backend. */
	EOnlineDataAdvertisementType::Type AdvertisementType;

	/** Default constructor. */
	FNexusLinkSessionSetting();

	/** Constructor from FVariantData. */
	FNexusLinkSessionSetting(const FName InKey, const FVariantData& InData, const EOnlineDataAdvertisementType::Type InAdvType);

	/** Templated constructor for arbitrary value types (int32, FString, float, etc). */
	template<typename ValueType>
	FNexusLinkSessionSetting(const FName InKey, const ValueType InValue, const EOnlineDataAdvertisementType::Type InAdvType)
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
struct NEXUSLINK_API FNexusLinkQuerySetting
{
	GENERATED_BODY()

	/** Setting key to compare against. */
	FName Key;

	/** The value to compare with. */
	FVariantData Data;

	/** How to compare the values. */
	EOnlineComparisonOp::Type ComparisonOp;

	/** Default constructor. */
	FNexusLinkQuerySetting();

	/** Constructor from FVariantData. */
	FNexusLinkQuerySetting(const FName InKey, const FVariantData& InData, const EOnlineComparisonOp::Type InCompOp);

	/** Templated constructor for arbitrary value types. */
	template<typename ValueType>
	FNexusLinkQuerySetting(const FName InKey, const ValueType InValue, const EOnlineComparisonOp::Type InCompOp)
		: Key(InKey)
		, Data(FVariantData())
		, ComparisonOp(InCompOp)
	{
		Data.SetValue(InValue);
	}

	/** @return Whether this query setting is valid. */
	bool IsValid() const
	{
		// We don't accept Near, In, NotIn comparison types — they require special handling.
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
			NEXUS_LOG(LogNexusLink, Error, TEXT("CompareAgainst called with nullptr!"));
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
struct NEXUSLINK_API FNexusLinkSessionSettings
{
	GENERATED_BODY()

	/** The underlying native session settings. Not exposed to Blueprint. */
	FOnlineSessionSettings SessionSettings;

	/** Server/Lobby display name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	FString ServerName;

	/** Map name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	FString MapName;

	/** Game mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	FString GameMode;

	/** Max session capacity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	int32 MaxNumPlayers;

	/** Whether the session is advertised. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	bool bShouldAdvertise;

	/** Whether the session is hidden from normal searches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	bool bHidden;

	/** Whether players can join after the game starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	bool bAllowJoinInProgress;

	/** Whether this is a LAN match. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	bool bIsLanMatch;

	/** Whether the session uses presence (lobbies). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	bool bUsesPresence;

	/** Whether invites are allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	bool bAllowInvites;

	/** Whether joining via presence is allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink")
	bool bAllowJoinViaPresence;

	/** Default constructor. */
	FNexusLinkSessionSettings();

	/** Constructor from native FOnlineSessionSettings. Extracts all known custom keys. */
	FNexusLinkSessionSettings(const FOnlineSessionSettings& InSessionSettings);

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
struct NEXUSLINK_API FNexusLinkHostParams
{
	GENERATED_BODY()

	/**
	 * The map to load via ServerTravel after session creation.
	 * Empty string = do not travel (stay on current map).
	 * Example: /Game/Maps/Dungeon_01
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	FString StartingLevel;

	/** Display name of the server/lobby. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	FString ServerName;

	/** Map name metadata stored on the session for display/filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	FString MapName;

	/** Game mode metadata stored on the session for display/filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	FString GameMode;

	/** Maximum number of players allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MaxNumPlayers;

	/** Whether the session should be publicly advertised. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	bool bShouldAdvertise;

	/** Whether the session is hidden from normal searches (invite-only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	bool bHidden;

	/** Whether players can join after the game has started. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	bool bAllowJoinInProgress;

	/** Whether this is a LAN match. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	bool bIsLanMatch;

	/** Whether the session uses presence (Steam lobbies, EOS lobbies). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	bool bUsesPresence;

	/** Whether invites to this session are allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	bool bAllowInvites;

	/** Whether players can join via presence (friend list join). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Host")
	bool bAllowJoinViaPresence;

	/** Additional custom session settings applied on top of the defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "NexusLink|Host")
	TArray<FNexusLinkSessionSetting> ExtraSessionSettings;

	/**
	 * Optional: override the FOnlineSessionSettings entirely.
	 * When set, the individual fields above are ignored and this is used directly.
	 * Not exposed to Blueprint — C++ power-user feature.
	 */
	TOptional<FOnlineSessionSettings> SessionSettingsOverride;

	/** Default constructor with sensible defaults for a co-op game. */
	FNexusLinkHostParams();

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
struct NEXUSLINK_API FNexusLinkSearchParams
{
	GENERATED_BODY()

	/** Maximum number of results to return. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Search", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxSearchResults;

	/** Whether to search for LAN sessions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Search")
	bool bIsLanQuery;

	/** Whether to search presence-based sessions (lobbies). Recommended for Steam/EOS. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusLink|Search")
	bool bSearchPresence;

	/** Extra query settings for server-side or local filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "NexusLink|Search")
	TArray<FNexusLinkQuerySetting> ExtraQuerySettings;

	/** List of session unique IDs to exclude from results. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "NexusLink|Search")
	TArray<FUniqueNetIdRepl> IgnoredSessions;

	/** Default constructor. */
	FNexusLinkSearchParams();

	/** @return Whether these search parameters pass validation. */
	bool IsValid(const bool bLogErrors = true) const;
};

/**
 * A single search result representing a found online session.
 * Wraps FOnlineSessionSearchResult with Blueprint-accessible helpers and sort predicates.
 */
USTRUCT(BlueprintType)
struct NEXUSLINK_API FNexusLinkSearchResult
{
	GENERATED_BODY()

	/** The underlying online search result. Not exposed to Blueprint. */
	FOnlineSessionSearchResult OnlineResult;

	/** Default constructor. */
	FNexusLinkSearchResult();

	/** Constructor from a native online search result. */
	FNexusLinkSearchResult(const FOnlineSessionSearchResult& InResult);

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
	FNexusLinkSessionSettings GetSessionSettings() const;

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
	static bool ComparePing(const FNexusLinkSearchResult& A, const FNexusLinkSearchResult& B)
	{
		return A.GetPing() < B.GetPing();
	}

	/** Sort by player count descending (fullest sessions first). */
	static bool ComparePlayerCountDesc(const FNexusLinkSearchResult& A, const FNexusLinkSearchResult& B)
	{
		return A.GetNumPlayers() > B.GetNumPlayers();
	}

	/** Sort by open slots ascending (least available first). */
	static bool CompareOpenSlotsAsc(const FNexusLinkSearchResult& A, const FNexusLinkSearchResult& B)
	{
		return A.GetNumOpenSlots() < B.GetNumOpenSlots();
	}
};

/**
 * Blueprint-friendly wrapper around FNamedOnlineSession.
 * Represents a session that the local player is currently part of.
 */
USTRUCT(BlueprintType)
struct NEXUSLINK_API FNexusLinkNamedSession
{
	GENERATED_BODY()

	/** Pointer to the native named session in the online subsystem. Not owned by us. */
	const FNamedOnlineSession* OnlineSession;

	/** Default constructor. */
	FNexusLinkNamedSession()
		: OnlineSession(nullptr)
	{}

	/** Constructor from a native named session. */
	FNexusLinkNamedSession(const FNamedOnlineSession* InSession)
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
	ENexusLinkSessionState GetSessionState() const
	{
		return IsValid() ? static_cast<ENexusLinkSessionState>(OnlineSession->SessionState) : ENexusLinkSessionState::NoSession;
	}

	/** @return Session settings wrapped in our struct. */
	FNexusLinkSessionSettings GetSessionSettings() const
	{
		return IsValid() ? FNexusLinkSessionSettings(OnlineSession->SessionSettings) : FNexusLinkSessionSettings();
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
struct NEXUSLINK_API FNexusLinkOnlineFriend
{
	GENERATED_BODY()

	/** Unique net ID of the friend. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "NexusLink")
	FUniqueNetIdRepl UserId;

	/** Display name of the friend. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "NexusLink")
	FString DisplayName;

	/** Current presence status. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "NexusLink")
	ENexusLinkFriendPresence Presence;

	/** Whether the friend is online (convenience, derived from Presence). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "NexusLink")
	bool bIsOnline;

	/** Whether the friend is playing our game (convenience, derived from Presence). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "NexusLink")
	bool bIsInGame;

	/** Default constructor. */
	FNexusLinkOnlineFriend();

	/** Constructor from a native FOnlineFriend. Extracts presence data automatically. */
	FNexusLinkOnlineFriend(const FOnlineFriend& NativeFriend);

	/** @return Whether this friend data is valid. */
	bool IsValid() const { return UserId.IsValid(); }
};

/**
 * Represents a session invite received by the local player.
 * Stored until the player accepts, declines, or the invite expires.
 */
USTRUCT(BlueprintType)
struct NEXUSLINK_API FNexusLinkPendingInvite
{
	GENERATED_BODY()

	/** Unique ID of the player who sent the invite. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "NexusLink")
	FUniqueNetIdRepl FromId;

	/** The session that the invite points to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "NexusLink")
	FNexusLinkSearchResult Session;

	/** Real time (seconds since app start) when the invite was received. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "NexusLink")
	float TimeReceived;

	/** Default constructor. */
	FNexusLinkPendingInvite()
		: FromId(FUniqueNetIdRepl())
		, Session(FNexusLinkSearchResult())
		, TimeReceived(0.0f)
	{}

	/** @return Whether the invite is valid. */
	bool IsValid() const { return FromId.IsValid() && Session.IsValid(); }
};

/** Fired when a session is created. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusLinkOnSessionCreated, ENexusLinkCreateSessionResult, Result);

/** Fired when sessions are found. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNexusLinkOnSessionsFound, ENexusLinkFindSessionsResult, Result, const TArray<FNexusLinkSearchResult>&, Results);

/** Fired when a session is joined. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusLinkOnSessionJoined, ENexusLinkJoinSessionResult, Result);

/** Fired when a session is destroyed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusLinkOnSessionDestroyed, ENexusLinkDestroySessionResult, Result);

/** Fired when the session state changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNexusLinkOnSessionStateChanged, FName, SessionName, ENexusLinkSessionState, NewState);

/** Fired when the friends list is read. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNexusLinkOnFriendsListReady, bool, bWasSuccessful, const TArray<FNexusLinkOnlineFriend>&, Friends);

/** Fired when a session invite is received from another player. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusLinkOnSessionInviteReceived, const FNexusLinkPendingInvite&, Invite);

/** Fired when the local user accepts a session invite (e.g. from Steam Overlay). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusLinkOnSessionInviteAccepted, const FNexusLinkSearchResult&, InviteResult);

// ============================================================================
// Delegates — Native (C++ only, for internal proxy binding)
// ============================================================================

DECLARE_MULTICAST_DELEGATE_OneParam(FNexusLinkNativeOnSessionCreated, ENexusLinkCreateSessionResult /*Result*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FNexusLinkNativeOnSessionsFound, ENexusLinkFindSessionsResult /*Result*/, const TArray<FNexusLinkSearchResult>& /*Results*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FNexusLinkNativeOnSessionJoined, ENexusLinkJoinSessionResult /*Result*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FNexusLinkNativeOnSessionDestroyed, ENexusLinkDestroySessionResult /*Result*/);
