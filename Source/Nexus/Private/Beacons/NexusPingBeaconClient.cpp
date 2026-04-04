#include "Beacons/NexusPingBeaconClient.h"
#include "NexusLog.h"
#include "NexusOnlineSettings.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NexusPingBeaconClient)

ANexusPingBeaconClient::ANexusPingBeaconClient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bContinuousPing(false)
	, PingInterval(2.0f)
	, MaxPingCount(1)
	, NextSequenceId(0)
	, CompletedPingCount(0)
{

}

bool ANexusPingBeaconClient::ConnectDirect(const FString& Address)
{
	// AOnlineBeacon::InitClient is the protected method used by APartyBeaconClient
	// for raw URL connections. We expose it here as a clean public entry point.
	FURL DefaultURL;
	FURL BeaconURL(&DefaultURL, *Address, TRAVEL_Absolute);

	if (!BeaconURL.Valid)
	{
		NEXUS_LOG(LogNexus, Error, TEXT("PingBeaconClient::ConnectDirect Invalid URL: '%s'"), *Address);
		return false;
	}

	if (!Address.Contains(TEXT(":")))
	{
		BeaconURL.Port = UNexusOnlineSettings::Get()->BeaconListenPort;
	}

	return InitClient(BeaconURL);
}

void ANexusPingBeaconClient::OnConnected()
{
	Super::OnConnected();

	NEXUS_LOG(LogNexus, Log, TEXT("PingBeaconClient connected. Starting ping sequence."));
	SendPing();
}

void ANexusPingBeaconClient::OnFailure()
{
	Super::OnFailure();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_Ping);
	}

	PendingPings.Empty();
	NEXUS_LOG(LogNexus, Warning, TEXT("PingBeaconClient connection failed."));
	OnPingFailureEvent.Broadcast();
}

void ANexusPingBeaconClient::SendPing()
{
	const uint32 SeqId = ++NextSequenceId;
	PendingPings.Add(SeqId, FPlatformTime::Seconds());
	ServerPing(SeqId);
}

void ANexusPingBeaconClient::ScheduleNextPing()
{
	++CompletedPingCount;

	const bool bLimitReached = (MaxPingCount > 0 && CompletedPingCount >= MaxPingCount);
	if (!bContinuousPing || bLimitReached)
	{
		DestroyBeacon();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Schedule the next ping after PingInterval seconds.
	// We re-schedule on each pong rather than looping, so the interval is measured
	// from the end of the previous round-trip (more accurate under high latency).
	World->GetTimerManager().SetTimer(TimerHandle_Ping, this, &ANexusPingBeaconClient::SendPing, PingInterval, false);
}

void ANexusPingBeaconClient::ServerPing_Implementation(uint32 SequenceId)
{
	// Server-side: simply echo the ID back. No server-side logic needed.
	ClientPong(SequenceId);
}

void ANexusPingBeaconClient::ClientPong_Implementation(uint32 SequenceId)
{
	const double* SendTime = PendingPings.Find(SequenceId);
	if (!SendTime)
	{
		// Can happen if the beacon was destroyed mid-flight.
		NEXUS_LOG(LogNexus, Verbose, TEXT("Received pong for unknown sequence %u - ignoring."), SequenceId);
		return;
	}

	const float PingMs = static_cast<float>((FPlatformTime::Seconds() - *SendTime) * 1000.0);
	PendingPings.Remove(SequenceId);

	NEXUS_LOG(LogNexus, Verbose, TEXT("%.2f ms (seq=%u, count=%d)"), PingMs, SequenceId, CompletedPingCount + 1);
	OnPingResultEvent.Broadcast(PingMs);
	ScheduleNextPing();
}