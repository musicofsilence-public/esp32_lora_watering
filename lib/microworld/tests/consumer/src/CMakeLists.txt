get_filename_component(
    MICROWORLD_CONSUMER_ENVIRONMENT
    "${BUILD_DIR}"
    NAME
)

# PlatformIO isolates each environment in a same-named build directory, which
# gives ESP-IDF one entry point even though it ignores build_src_filter.
if(MICROWORLD_CONSUMER_ENVIRONMENT STREQUAL "esp32-s3")
    set(MICROWORLD_CONSUMER_SOURCE Esp32Main.cpp)
elseif(MICROWORLD_CONSUMER_ENVIRONMENT STREQUAL "esp32-s3-benchmark")
    set(MICROWORLD_CONSUMER_SOURCE Esp32BenchmarkMain.cpp)
else()
    message(FATAL_ERROR
        "Unsupported MicroWorld ESP32 consumer environment: "
        "${MICROWORLD_CONSUMER_ENVIRONMENT}")
endif()

idf_component_register(SRCS "${MICROWORLD_CONSUMER_SOURCE}")
