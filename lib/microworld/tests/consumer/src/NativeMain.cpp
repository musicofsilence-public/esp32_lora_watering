#include <MicroWorld/Application.h>
#include <MicroWorld/Version.h>
#include <MicroWorld/World.h>

#include <type_traits>

static_assert(__cplusplus >= 201703L);
static_assert(std::is_nothrow_destructible_v<MicroWorld::FApplication>);
static_assert(MicroWorld::Version.Major == 0);
static_assert(MicroWorld::Version.Minor == 1);
static_assert(MicroWorld::Version.Patch == 0);

/** Proves a downstream host executable can link and run the exact public package. */
int main()
{
	MicroWorld::TWorld<1> ConsumerCompileProbe;
	return ConsumerCompileProbe.BeginPlay(0) == MicroWorld::ERuntimeResult::Success
			&& ConsumerCompileProbe.EndPlay() == MicroWorld::ERuntimeResult::Success
		? 0
		: 1;
}
