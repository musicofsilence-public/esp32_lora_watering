get_filename_component(
    MICROWORLD_CONSUMER_ENVIRONMENT
    "${BUILD_DIR}"
    NAME
)

# PlatformIO isolates each environment in a same-named build directory, which
# gives ESP-IDF one entry point even though it ignores build_src_filter.
if(MICROWORLD_CONSUMER_ENVIRONMENT STREQUAL "esp32-s3")
    set(MICROWORLD_CONSUMER_SOURCE Esp32Main.cpp)
elseif(MICROWORLD_CONSUMER_ENVIRONMENT STREQUAL "esp32-s3-memory")
    set(MICROWORLD_CONSUMER_SOURCE MemoryEsp32Main.cpp)
elseif(MICROWORLD_CONSUMER_ENVIRONMENT STREQUAL "esp32-s3-object")
    set(MICROWORLD_CONSUMER_SOURCE ObjectEsp32Main.cpp)
elseif(MICROWORLD_CONSUMER_ENVIRONMENT STREQUAL "esp32-s3-benchmark")
    set(MICROWORLD_CONSUMER_SOURCE Esp32BenchmarkMain.cpp)
else()
    message(FATAL_ERROR
        "Unsupported MicroWorld ESP32 consumer environment: "
        "${MICROWORLD_CONSUMER_ENVIRONMENT}")
endif()

idf_component_register(SRCS "${MICROWORLD_CONSUMER_SOURCE}")

if(MICROWORLD_CONSUMER_ENVIRONMENT STREQUAL "esp32-s3-memory" OR MICROWORLD_CONSUMER_ENVIRONMENT STREQUAL "esp32-s3-object")
    # PlatformIO forwards project build flags to ESP-IDF C sources as well as
    # this C++ component. Keep C++-only portability gates on the target that
    # instantiates Memory's representative public templates. GCC 15.2's
    # libstdc++ diagnoses std::aligned_storage in the C++17 StaticVector and
    # Delegate headers as deprecated; suppress only that declaration warning
    # until the separately owned production headers adopt replacement storage.
    target_compile_options(
        ${COMPONENT_LIB}
        PRIVATE
            -std=gnu++17
            -Wall
            -Wextra
            -Wpedantic
            -Werror
            -Wno-deprecated-declarations
            -fno-exceptions
            -fno-rtti
    )
endif()
