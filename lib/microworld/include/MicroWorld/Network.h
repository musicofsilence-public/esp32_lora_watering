#pragma once

#include <MicroWorld/Lifecycle.h>
#include <MicroWorld/Tickable.h>

namespace MicroWorld
{

/** Defines policy-free lifecycle and ticking for a consumer-owned network subsystem. */
class FNetwork : public FTickable
{
public:
	/** Preserves the address referenced by a consumer composition root. */
	FNetwork(const FNetwork&) = delete;

	/** Prevents lifecycle and transport-observation state from being duplicated. */
	FNetwork& operator=(const FNetwork&) = delete;

	/** Preserves the address referenced by a consumer composition root. */
	FNetwork(FNetwork&&) = delete;

	/** Prevents moving an active network boundary behind consumer references. */
	FNetwork& operator=(FNetwork&&) = delete;

	/** Starts the consumer hook and independent primary tick. */
	ERuntimeResult BeginPlay(TimePointMilliseconds NowMilliseconds) noexcept;

	/** Runs the network hook at its configured cadence. */
	ERuntimeResult Advance(TimePointMilliseconds NowMilliseconds) noexcept;

	/** Ends the consumer hook once and is idempotent after success. */
	ERuntimeResult EndPlay() noexcept;

protected:
	/** Gives the consumer-selected network cadence its own primary schedule. */
	explicit FNetwork(FTickConfiguration TickConfiguration) noexcept;

	/** Allows consumer-owned derived network boundaries to be destroyed safely. */
	virtual ~FNetwork() = default;

	/** Lets a derived boundary initialize transport without adding policy to MicroWorld. */
	virtual void OnNetworkBeginPlay() {}

	/** Lets a derived boundary perform bounded transport work when its schedule is due. */
	virtual void TickNetwork(const FTickContext& Context) = 0;

	/** Lets a derived boundary release transport resources before schedule shutdown. */
	virtual void OnNetworkEndPlay() {}

private:
	/** Keeps network lifecycle independent from World and Actor lifecycles. */
	FLifecycleGuard Lifecycle;
};

} // namespace MicroWorld
