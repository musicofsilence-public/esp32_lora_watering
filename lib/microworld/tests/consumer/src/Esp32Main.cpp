#include <MicroWorld/Version.h>
#include <MicroWorld/World.h>

static_assert(MicroWorld::Version.Major == 0);
static_assert(MicroWorld::Version.Minor == 1);
static_assert(MicroWorld::Version.Patch == 0);

/** Proves ESP-IDF can link the exact package without platform dependencies entering it. */
extern "C" void app_main()
{
	MicroWorld::TWorld<1> ConsumerCompileProbe;
	(void)ConsumerCompileProbe.BeginPlay(0);
	(void)ConsumerCompileProbe.EndPlay();
}
