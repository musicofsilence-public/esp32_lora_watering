/*
 * First experiment: blink an external LED.
 *
 * Wire the circuit like this:
 *
 *     GPIO10 -> 220-330 ohm resistor -> LED long leg (+)
 *     LED short leg (-) -> GND
 *
 * The resistor limits the current and protects both the LED and the ESP32 pin.
 */

// Provides fixed-width integer types such as std::uint32_t.
#include <cstdint>

// Provides the ESP-IDF functions used to control GPIO pins.
#include "driver/gpio.h"

// Provides ESP_ERROR_CHECK(), which stops with a useful error if setup fails.
#include "esp_err.h"

// Provides FreeRTOS types and the pdMS_TO_TICKS() time conversion.
#include "freertos/FreeRTOS.h"

// Provides vTaskDelay(), which pauses this task without busy-waiting.
#include "freertos/task.h"

namespace {

// GPIO10 is the pin selected for this external-LED experiment.
constexpr gpio_num_t kLedGpio = GPIO_NUM_10;

// Each ON and OFF part of the blink lasts 1,000 milliseconds (one second).
constexpr std::uint32_t kBlinkDelayMs = 1'000;

}  // namespace

/*
 * ESP-IDF starts the application by calling app_main().
 *
 * extern "C" gives the function the C-compatible name expected by ESP-IDF,
 * even though this source file is compiled as C++.
 */
extern "C" void app_main() {
    /*
     * Return GPIO10 to its default state before configuring it. Starting from a
     * known state makes the experiment behave consistently after a reset.
     *
     * The result is stored before it is checked so each operation is visible
     * on its own line. This is easier to inspect while learning than a nested
     * call such as ESP_ERROR_CHECK(gpio_reset_pin(...)).
     */
    const esp_err_t reset_result = gpio_reset_pin(kLedGpio);
    ESP_ERROR_CHECK(reset_result);

    /*
     * Configure GPIO10 as an output so the program can drive the LED. Again,
     * perform the operation first and check its result on the following line.
     */
    const esp_err_t direction_result =
        gpio_set_direction(kLedGpio, GPIO_MODE_OUTPUT);
    ESP_ERROR_CHECK(direction_result);

    /*
     * FreeRTOS measures delays in scheduler ticks rather than milliseconds.
     * Convert the one-second delay once because its value never changes.
     */
    const TickType_t blink_delay_ticks = pdMS_TO_TICKS(kBlinkDelayMs);

    // Repeat forever because an embedded application normally keeps running.
    while (true) {
        /*
         * A HIGH level places about 3.3 V on GPIO10. With the wiring shown
         * above, current flows through the LED and it turns on.
         */
        const esp_err_t led_on_result = gpio_set_level(kLedGpio, 1);
        ESP_ERROR_CHECK(led_on_result);

        /*
         * Pause for one second. FreeRTOS can run other tasks during this delay,
         * unlike a CPU-wasting empty delay loop.
         */
        vTaskDelay(blink_delay_ticks);

        /*
         * A LOW level places about 0 V on GPIO10, stopping the current and
         * turning the LED off.
         */
        const esp_err_t led_off_result = gpio_set_level(kLedGpio, 0);
        ESP_ERROR_CHECK(led_off_result);

        // Keep the LED off for one second before the loop starts again.
        vTaskDelay(blink_delay_ticks);
    }
}
