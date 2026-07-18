cmake_minimum_required(VERSION 3.16)

project(MicroWorld VERSION 0.1.0 LANGUAGES CXX)

option(MICROWORLD_BUILD_TESTS "Build MicroWorld host tests" ON)
option(MICROWORLD_BUILD_BENCHMARKS "Build MicroWorld host benchmarks" ON)
option(MICROWORLD_BUILD_EXAMPLES "Build MicroWorld examples" ON)

add_library(microworld STATIC
    src/Application.cpp
    src/Actor.cpp
    src/ActorComponent.cpp
    src/Network.cpp
    src/TickFunction.cpp
)
target_include_directories(microworld PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_compile_features(microworld PUBLIC cxx_std_17)

# Keeps production, tests, probes, benchmarks, and examples under one warning gate.
function(microworld_enable_strict_warnings Target)
    if(MSVC)
        target_compile_options(${Target} PRIVATE /W4 /WX /Zc:__cplusplus)
    else()
        target_compile_options(${Target} PRIVATE
            -Wall -Wextra -Wpedantic -Werror
        )
    endif()
endfunction()

microworld_enable_strict_warnings(microworld)

if(MICROWORLD_BUILD_TESTS)
    include(CTest)
    add_executable(microworld_tests
        tests/TestMain.cpp
        tests/TickFunctionTests.cpp
        tests/WorldTests.cpp
        tests/ApplicationNetworkTests.cpp
    )
    target_link_libraries(microworld_tests PRIVATE microworld)
    microworld_enable_strict_warnings(microworld_tests)
    add_test(NAME microworld_tests COMMAND microworld_tests)

    add_executable(microworld_no_exceptions_rtti_probe
        tests/consumer/src/NativeMain.cpp
    )
    target_link_libraries(microworld_no_exceptions_rtti_probe
        PRIVATE microworld
    )
    microworld_enable_strict_warnings(
        microworld_no_exceptions_rtti_probe
    )
    if(MSVC)
        target_compile_options(microworld_no_exceptions_rtti_probe
            PRIVATE /GR-
        )
    else()
        target_compile_options(microworld_no_exceptions_rtti_probe
            PRIVATE -fno-exceptions -fno-rtti
        )
    endif()
endif()

if(MICROWORLD_BUILD_BENCHMARKS)
    add_executable(microworld_dispatch_benchmark
        benchmarks/DispatchBenchmark.cpp
    )
    target_link_libraries(microworld_dispatch_benchmark PRIVATE microworld)
    microworld_enable_strict_warnings(microworld_dispatch_benchmark)
endif()

if(MICROWORLD_BUILD_EXAMPLES)
    add_executable(microworld_host_lifecycle
        examples/HostLifecycle/Main.cpp
    )
    target_link_libraries(microworld_host_lifecycle PRIVATE microworld)
    microworld_enable_strict_warnings(microworld_host_lifecycle)
endif()
