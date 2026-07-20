#include "CoreConsumerProbe.h"

/**
 * Release-profile Core primitive link/size probe.
 *
 * The previous Core World/Actor/Component dispatch benchmark was retired with
 * the Core actor model (see MICROWORLD_ROADMAP.md Phase 1). Managed-engine
 * runtime measurement is re-established in Phase 6; until then this release
 * (-Os) ESP-IDF environment keeps a Core primitive compile/link probe.
 */

namespace
{

/** Retains the compile probe outcome so optimization cannot erase representative public calls. */
volatile int CoreConsumerProbeResult = -1;

} // namespace

extern "C" void app_main()
{
	CoreConsumerProbeResult = RunCoreConsumerProbe();
}
