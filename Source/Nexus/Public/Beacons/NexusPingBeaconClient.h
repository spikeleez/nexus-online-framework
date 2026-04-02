#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "NexusPingBeaconClient.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusOnPingResultSignature, float, PingTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNexusOnPingFailureSignature);

/**
 * @class UNexusPingBeaconClient 
 * 
 * Measures round-trip time (RTT) to a server using a lightweight beacon connection.
 *
 * Uses a sequence-ID echo pattern: the send timestamp is stored locally on the client,
 * only the ID travels over the wire. This eliminates floating-point precision loss
 * that would occur if sending timestamps as float RPCs.
 */
UCLASS(Blueprintable, Transient)
class NEXUS_API ANexusPingBeaconClient : public AOnlineBeaconClient
{
	GENERATED_BODY()

public:
	/** Fired each time an RTT measurement completes. PingMs is round-trip time in milliseconds. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events", meta = (DisplayName = "On Ping Result"))
	FNexusOnPingResultSignature OnPingResultEvent;

	/** Fired when the connection fails to establish or is unexpectedly dropped. */
	UPROPERTY(BlueprintAssignable, Category = "Nexus|Events", meta = (DisplayName = "On Ping Failure"))
	FNexusOnPingFailureSignature OnPingFailureEvent;

public:
	ANexusPingBeaconClient(const FObjectInitializer& ObjectInitializer);

	/**
	 * Connect directly via a raw address string (IP or hostname).
	 * Used for non-session connections (LAN, dedicated servers without OSS).
	 */
	bool ConnectDirect(const FString& Address);

public:
	/** When true: ping repeatedly at PingInterval. When false: ping once and self-destruct. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping")
	uint8 bContinuousPing:1;

	/** Seconds between successive pings. Minimum 0.5s. Only used when bContinuousPing is true. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping", meta = (ClampMin = "0.5", EditCondition = "bContinuousPing"))
	float PingInterval;

	/** Maximum pings before auto-disconnecting. 0 = unlimited. Only used when bContinuousPing is true. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping", meta = (ClampMin = "0", EditCondition = "bContinuousPing"))
	int32 MaxPingCount;

protected:
	//~Begin AOnlineBeaconClient interface
	virtual void OnConnected() override;
	virtual void OnFailure() override;
	//~End of AOnlineBeaconClient interface

	/** Sends a ping to the server. Called automatically on OnConnected. */
	virtual void SendPing();

	/**
	 * Called after each pong response. Decides whether to schedule another ping or self-destruct.
	 * Override to inject custom inter-ping logic.
	 */
	virtual void ScheduleNextPing();

private:
	/** Server echoes SequenceId back untouched. No payload other than the ID. */
	UFUNCTION(Server, Reliable)
	void ServerPing(uint32 SequenceId);

	/** Client receives echo and computes RTT from the locally stored send time. */
	UFUNCTION(Client, Reliable)
	void ClientPong(uint32 SequenceId);

private:
	/** Maps in-flight sequence ID -> high-precision send time (FPlatformTime::Seconds). */
	TMap<uint32, double> PendingPings;

	uint32 NextSequenceId;
	int32 CompletedPingCount;
	FTimerHandle TimerHandle_Ping;
};
