# ESP32 + LoRa E32 Watering System — Daily Guide (Plain Language Edition)

This guide takes you from zero ESP32 knowledge to a working remote watering system.
It is split into 74 days. One day is about 30 minutes of work.
Each day is self-contained. It gives you: the goal, the theory in simple words, the reason for each design choice, the code, and a clear test to know you are done.

## How to use this guide

1. **One day = about 30 minutes.** Work 5 days per week. The full plan takes about 15 weeks.
2. **If a day takes longer, continue it tomorrow.** Never skip the "Check" step. If you skip it, a hidden problem will appear later, when it is much harder to find.
3. **Day markers:**
   - 💻 = computer only. No hardware needed.
   - 🔧 = hardware day. Plan 45–60 minutes, or do it on a weekend.
   - ⚡ = day with the 12 V valve circuit. Work slowly. Never do this when tired.
4. **Learning method:** before you run any code, write down what you expect it to print. Then run it. If the result is different, find out why. This difference is exactly the thing you do not understand yet. This method is the core of the whole guide.
5. Keep a file `LEARNING_LOG.md` in your project. Write one or two lines per day: what surprised you, and the numbers you measured.

## Hardware you need

- 2 × ESP32 DevKit boards (with the ESP32-WROOM-32 module)
- 2 × E32 LoRa modules (868 MHz or 433 MHz, depending on your country) **with antennas**
- **Safety rule: never transmit without an antenna.** The transmitter power has nowhere to go and can destroy the module.
- 1 × capacitive soil moisture sensor
- 1 × logic-level N-MOSFET (for example IRLZ44N), 1 × diode (1N5819 or 1N4007)
- 1 × 12 V solenoid valve and a 12 V power supply
- 1 × capacitor 470 µF or bigger
- Resistors: 100 Ω, 100 kΩ, 2 × 470 kΩ
- Breadboard, wires, a multimeter

## Small dictionary (terms used in this guide)

- **Firmware** — the program that runs on the microcontroller.
- **Flash** — the permanent memory chip where the firmware is stored.
- **Partition** — a named region of flash with a fixed purpose (for example: program, settings).
- **Task** — a function that runs like an independent small program, managed by the operating system.
- **FreeRTOS** — the small operating system that runs on the ESP32 and manages tasks.
- **Queue** — a safe "pipe" used to send data from one task to another.
- **Mutex** — a lock. Only one task can hold it at a time.
- **ISR (interrupt handler)** — a small function that hardware calls immediately when an event happens (for example, a pin changes).
- **UART** — a simple serial connection with two wires (TX = transmit, RX = receive).
- **ADC** — the circuit that converts a voltage into a number.
- **NVS** — "Non-Volatile Storage". A key-value database in flash for settings.
- **CRC** — a mathematical checksum that detects damaged data.
- **ACK** — a short "I received it" reply message.
- **Duty cycle** — the legal limit on how much time per hour a radio may transmit.
- **Deep sleep** — a power mode where almost the whole chip is off.
- **MQTT** — a simple network protocol for sending small messages between devices through a central server.

---

# WEEK 1 — The tools
**Goal of the week:** a professional project setup. At the end, you can explain what happens when you press "Upload".

### Day 1 💻🔧 — Install the tools
**Goal:** VS Code and PlatformIO installed. A test program runs on the board.

**Why PlatformIO and not Arduino IDE?** You already know Arduino IDE, so this choice needs a reason. There are four:
1. *Library versions.* Arduino IDE installs libraries globally, without exact versions. PlatformIO writes the exact version of every library into a text file inside the project. Result: the same project builds the same firmware today and in two years. Commercial work requires this.
2. *Two programs in one project.* Your system has two different firmwares (field node and base station) that share code. PlatformIO supports this directly. Arduino IDE does not.
3. *Testing.* PlatformIO can run automatic tests, even on your PC.
4. *Automation.* PlatformIO works from the command line, so a build server can build and test every change.

**One important fact before you start.** Espressif (the maker of the ESP32) stopped supporting PlatformIO officially. Because of this, the standard PlatformIO ESP32 platform is stuck on the old Arduino core version 2. The new core version 3 is available through a community project called **pioarduino**. We will use pioarduino. Remember this fact: many tutorials on the internet were written for the old core, and some function names changed. When code from the internet does not compile, first ask: "which core version was this written for?"

**Do:**
1. Install VS Code.
2. In VS Code, open Extensions and install "PlatformIO IDE".
3. Create a new project: board `esp32dev`, framework Arduino.
4. Build and upload the default blink example.

**Check:** the LED blinks, and you uploaded it from VS Code, not from Arduino IDE.

### Day 2 💻 — The project configuration file
**Goal:** understand every line of `platformio.ini`.

**Concept.** `platformio.ini` describes the whole build: which board, which libraries with which versions, and which compiler options. Each `[env:...]` section is one separate firmware. Our project needs two.

**Do.** Replace the content of `platformio.ini` with this:

```ini
[platformio]
default_envs = field_node

[env]
; This line selects the community "pioarduino" platform (Arduino core 3).
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = esp32dev
framework = arduino
monitor_speed = 115200
; exception_decoder: when the program crashes, this prints a readable
; list of function names instead of raw numbers.
; time: adds a timestamp to every serial line.
monitor_filters = esp32_exception_decoder, time
build_flags =
    -DCORE_DEBUG_LEVEL=3
    -Wall -Wextra

[env:field_node]
build_src_filter = +<common/> +<field_node/>
lib_deps = xreef/EByte LoRa E32 library@^1.5.13

[env:base_station]
build_src_filter = +<common/> +<base_station/>
lib_deps =
    xreef/EByte LoRa E32 library@^1.5.13
    knolleary/PubSubClient@^2.8
```

Create this folder structure:

```
project/
├── platformio.ini
├── lib/                     ← your own shared libraries
├── src/
│   ├── common/
│   ├── field_node/main.cpp
│   └── base_station/main.cpp
└── test/
```

Put an empty `void setup(){} void loop(){}` into both main.cpp files.
Then, for every line in the ini file, write one sentence in your log that explains it. Do this from memory.

**Check:** both commands build without errors:
`pio run -e field_node` and `pio run -e base_station`.

### Day 3 💻 — What "Upload" really does
**Goal:** understand the path from source code to a running chip.

**Concept.** Five steps happen:
1. A **cross-compiler** (`xtensa-esp32-elf-g++`) turns each `.cpp` file into machine code for the ESP32 processor.
2. The **linker** joins all pieces into one program file. It uses a memory map of the ESP32 to decide where each part will live.
3. The tool **esptool** sends the program to the chip over USB. It talks to a small loader program that is burned into the chip at the factory and can never be erased (the "ROM bootloader").
4. Flash does not hold one single file. It holds several images at fixed addresses: a second-stage bootloader at address 0x1000, a **partition table** at 0x8000, and your program at 0x10000. The partition table is a small map: "the program is here, the settings storage is here". We will use this map again in Week 6 (settings) and after the project (firmware updates over the air).
5. After reset, the chip starts the ROM bootloader → it starts the second-stage bootloader → it reads the partition table → it finds your program → it runs it. Important detail: your program mostly runs **directly from flash, through a cache**. This is why writing to flash briefly freezes the program. This fact will matter twice later.

**Do.** Run `pio run -v` (verbose build). Find in the output: one compiler call, the linker step, and the esptool line with the three flash addresses. Then draw the boot sequence from memory.

**Check:** your drawing matches the five steps above without looking.

### Day 4 🔧 — First useful program, and version control
**Goal:** a diagnostic program runs; the project is in git.

**Concept.** The function `esp_reset_reason()` reports *why* the chip restarted: normal power-on, software restart, **brownout** (the supply voltage dropped too low), watchdog (a safety timer fired), or wake from deep sleep. For a device installed in a garden, this one function is your most valuable diagnostic tool. We introduce it on day 4 because you will use it for the rest of the project.

**Do.** Flash this to the field_node environment and open the monitor:

```cpp
#include <Arduino.h>
void setup() {
    Serial.begin(115200);
    delay(300);                       // give the USB-serial chip time to start
    Serial.printf("Reset reason: %d\n", esp_reset_reason());
    Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
    Serial.printf("CPU: %u MHz, Flash: %u\n", getCpuFrequencyMhz(), ESP.getFlashChipSize());
}
void loop() {
    static uint32_t n = 0;
    Serial.printf("[%lu] alive %u\n", millis(), n++);
    delay(1000);
}
```

Then: `git init`, create `.gitignore` with the line `.pio/`, and make the first commit.

**Check:** the board prints its numbers; `git log` shows one commit.

### Day 5 🔧 — Learn three failures in a safe place
**Goal:** see three common failures now, so you recognize them instantly later.

**Do:**
1. Set a wrong `board =` value and build. Read the whole error message, not only the last line.
2. Run `pio run -t erase`. Open the monitor on the empty chip. Then flash again.
3. Set `monitor_speed = 9600` and open the monitor. You will see garbage characters. This is what a wrong serial speed looks like. Memorize this picture. Restore 115200.

**Check:** three lines in your log, each in the form "symptom → cause".

### Day 6 💻 — Self-test for Week 1
Answer in writing, without looking:
1. Why do exact library versions matter in commercial work?
2. What is the partition table, and at which flash address is it stored?
3. How can a program run "from flash"? What is the price of this?
4. What is pioarduino, and why does it exist?

If an answer is weak, re-read only that day. Also finish anything left from this week.
**Check:** four confident written answers.

---

# WEEK 2 — Know your chip
**Goal of the week:** a pin plan for the whole project, with a reason for every pin.

### Day 7 💻 — What is inside the ESP32
**Goal:** replace your old Arduino (AVR) mental model with the correct one.

**Concept.** The ESP32 is a different class of device:
- **Two processor cores** at 240 MHz. Core 0 runs the WiFi and Bluetooth software. Core 1 runs your Arduino `setup()` and `loop()`. Important: your `loop()` is not "the program". It is just one **task** inside an operating system (FreeRTOS) that is already running. Week 3 is fully about the consequences of this fact.
- About 520 KB of RAM and about 4 MB of flash.
- A small, always-powered section called the **RTC domain**, with its own 8 KB memory. This is the hardware that makes deep sleep possible (Week 11).
- One built-in 2.4 GHz radio for WiFi and Bluetooth. Your E32 is a separate radio on a different frequency. They do not disturb each other.
- Many peripherals: 3 UARTs, SPI, I2C, a multi-channel ADC, and more.

**Do.** Write five things that were true on your AVR projects but are false here, and what replaces each one.

**Check:** five written contrasts.

### Day 8 💻 — Pins: which ones you may use, and why
**Goal:** a complete pin plan for the project.

**Concept.** On the ESP32, almost any internal peripheral can be connected to almost any pin through an internal switch matrix. This is why `Serial2.begin(baud, SERIAL_8N1, RX_PIN, TX_PIN)` accepts pin numbers. But some pins have hardware restrictions:

| Pins | Restriction | Reason |
|---|---|---|
| 34, 35, 36, 39 | Input only. No internal pull-up resistors. | These pins physically have no output driver circuit. |
| 0, 2, 5, 12, 15 | "Strapping" pins. | The chip reads them at reset to select the boot mode. Your circuit must not force them to a wrong level at boot. Example: pin 12 held high at boot selects a wrong flash voltage, and the board looks dead. |
| 6–11 | Never use. | They connect the internal flash chip. |
| 1, 3 | UART0. | This is your serial monitor. |
| ADC2 group (0, 2, 4, 12–15, 25–27) | The ADC on these pins stops working while WiFi is on. | The WiFi driver uses the same ADC hardware. Analog sensors go on ADC1 pins: 32–39. |

**Do.** Draw your board's pinout and mark the restrictions. Then check this pin plan against the table. All code in this guide uses it:

| Signal | GPIO | Why this pin is safe |
|---|---|---|
| E32 RX (ESP32 TX2) | 17 | free pin, output allowed |
| E32 TX (ESP32 RX2) | 16 | free pin |
| E32 M0 | 21 | free output |
| E32 M1 | 19 | free output |
| E32 AUX | 18 | input, any pin works |
| Soil sensor signal | 34 | ADC1 group; "input only" is fine for a sensor |
| Soil sensor power | 25 | output; it is an ADC2 pin, but we use it only as a digital output |
| Valve MOSFET | 26 | normal output, not a strapping pin |
| Battery measurement | 35 | ADC1 group, input only |

**Check:** every row in your table has a written reason.

### Day 9 🔧 — Three kinds of memory, and one silent failure
**Goal:** know where code and data live, and see a failure that produces no error.

**Concept.** Four memory regions matter to you:
- **DRAM** — normal data: variables, heap, task stacks.
- **IRAM** — a small RAM for code that must run even when flash is temporarily unavailable. The main example: an interrupt handler that fires *during* a flash write. Marking an ISR with `IRAM_ATTR` places it in IRAM. Without it, that situation crashes the chip. So `IRAM_ATTR` is protection, not style.
- **RTC memory** — 8 KB that stays powered in deep sleep. Used with `RTC_DATA_ATTR` (Week 11).
- **Flash (through cache)** — your program and constant data.

**Do.** Run this and measure pin 34 with a multimeter:

```cpp
pinMode(34, OUTPUT);        // pin 34 has no output driver
digitalWrite(34, HIGH);     // nothing will happen — and no error either
```

**Check:** you observed that nothing happens, and you wrote in the log why a silent failure is worse than a crash (a crash tells you where the problem is; a silent failure hides).

### Day 10 💻 — Self-test for Week 2
Answer in writing:
1. Which pins can never be outputs, and why?
2. What is a strapping pin? What is the pin 12 problem?
3. What crash does `IRAM_ATTR` on an interrupt handler prevent?
4. Which core runs `loop()`? What runs on the other core?
Also: draw the Day 3 boot sequence again. Is it still in your memory?
**Check:** confident answers; week finished.

---

# WEEKS 3–4 — FreeRTOS: the operating system under your code
**Goal of the fortnight:** you have seen a data race with your own eyes, fixed it with a queue, used a mutex and an interrupt correctly, and chosen task stack sizes based on measurement. This is the most important part of the whole guide. Everything professional on the ESP32 stands on it.

### Day 11 💻 — How the scheduler works
**Goal:** understand tasks, priorities, and blocking.

**Concept.** A **task** is a function with its own stack. It runs as if it owns the whole processor. The **scheduler** is the part of FreeRTOS that decides which task really runs. Every millisecond (one "tick"), the scheduler gives the CPU to the task with the highest priority that is ready to run.

Three ideas to absorb:
1. `delay(100)` no longer wastes CPU time. On ESP32 it means: "block my task for 100 ms". While your task is blocked, other tasks run. **Blocking is cooperation, not waste.** This is the opposite of the AVR world.
2. Priority is absolute. A higher-priority task that becomes ready *always* interrupts a lower one. A high-priority task that never blocks will starve all other tasks. FreeRTOS detects this with a watchdog timer and restarts the chip. This restart is a safety feature.
3. Two cores means two tasks can run at the *same physical moment*. Any variable used by two tasks can now be corrupted. Tomorrow you will see this happen.

**Do.** Write in your log, in your own words: (a) why `delay()` is not wasteful anymore; (b) what happens to the system if one high-priority task never blocks.

**Check:** both explanations written.

### Day 12 🔧 — See a data race with your own eyes
**Goal:** watch shared memory get corrupted, and understand exactly why.

**Concept.** The C++ line `counter++` is not one operation. The processor executes three steps: load the value, add one, store the value back. If two cores do this at the same moment, both can load "100" and both store "101". One increment is lost. Nothing crashes. The data is simply wrong, quietly.

**Do.** First write down what you expect this program to print. Then run it three times:

```cpp
#include <Arduino.h>
uint32_t counter = 0;                     // shared by two tasks — intentionally unsafe

void incTask(void *name) {
    for (int i = 0; i < 100000; i++) counter++;   // load, add, store
    Serial.printf("%s done\n", (const char*)name);
    vTaskDelete(NULL);                    // a task must delete itself, never just return
}
void setup() {
    Serial.begin(115200); delay(300);
    // arguments: function, name, stack size in BYTES, parameter, priority, handle, core number
    xTaskCreatePinnedToCore(incTask, "A", 4096, (void*)"A", 1, NULL, 0);
    xTaskCreatePinnedToCore(incTask, "B", 4096, (void*)"B", 1, NULL, 1);
    delay(2000);
    Serial.printf("counter = %lu (expected 200000)\n", counter);
}
void loop() {}
```

**Check:** you recorded three different wrong totals, and you drew a small timeline of two cores showing the exact moment an increment is lost.

### Day 13 🔧 — Queues: the correct way to share data
**Goal:** build the producer–consumer pattern. You will use it again in Week 13.

**Concept.** The rule for this whole project: **tasks do not share variables; tasks send messages.** A **queue** is a thread-safe first-in-first-out buffer. It copies data *by value*, so sender and receiver never touch the same memory.

**Why a queue and not a shared variable with a flag?** Three reasons. First, the queue is safe by design; a shared variable needs manual protection, and Day 12 showed what happens without it. Second, a task can *block* on a queue: "sleep until data arrives". It uses zero CPU while waiting. A flag must be checked in a loop, which wastes CPU. Third, a queue keeps history: if the producer is briefly faster, messages wait in order instead of being overwritten.

**Do.**

```cpp
#include <Arduino.h>
struct Reading { uint32_t ms; uint16_t mv; };
QueueHandle_t q;

void producer(void*) {
    for (;;) {
        Reading r { millis(), (uint16_t)analogReadMilliVolts(34) };
        xQueueSend(q, &r, portMAX_DELAY);      // copies r into the queue
        vTaskDelay(pdMS_TO_TICKS(500));        // blocked: costs no CPU
    }
}
void consumer(void*) {
    Reading r;
    for (;;)
        if (xQueueReceive(q, &r, portMAX_DELAY) == pdTRUE)   // sleeps here until data arrives
            Serial.printf("[%lu] %u mV\n", r.ms, r.mv);
}
void setup() {
    Serial.begin(115200);
    q = xQueueCreate(10, sizeof(Reading));     // 10 slots, one struct per slot
    xTaskCreatePinnedToCore(producer, "p", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(consumer, "c", 4096, NULL, 1, NULL, 1);
}
void loop() { vTaskDelete(NULL); }             // we do not need the default loop task
```

Notice what this code does *not* contain: no `volatile` flags, no polling loop, no shared variable.

**Check:** it runs, and you can explain why the waiting consumer uses zero CPU.

### Day 14 🔧 — What happens when a queue is full
**Goal:** learn the two "queue full" policies. You will need this exact decision in Week 13.

**Concept.** When a queue is full, the sender must choose:
- `xQueueSend(q, &r, 0)` — do not wait. The send fails immediately, and the message is dropped.
- `xQueueSend(q, &r, portMAX_DELAY)` — wait as long as needed. The sender stalls until there is space.

Neither option is "correct" in general. It is a decision per message type:
- **Sensor data:** drop it. A new reading comes soon anyway. An old soil reading has no value.
- **Commands:** never drop silently. A lost "water the plants" command means dry plants and no error message.

**Do.** Make the consumer slow (`vTaskDelay(pdMS_TO_TICKS(2000))`), shrink the queue to 3 slots, and count the drops:

```cpp
if (xQueueSend(q, &r, 0) != pdTRUE) dropped++;
```

Then switch to `portMAX_DELAY` and watch the producer stall instead. Write the policy note in your log: "telemetry drops; commands wait."

**Check:** you saw both behaviors and wrote the policy.

### Day 15 🔧 — Mutex: when two tasks must share one device
**Goal:** protect a shared resource with a lock.

**Concept.** Some things cannot be sent through a queue. Example: the single UART wire that connects the E32 module. If two tasks write to it at the same time, the bytes mix. A **mutex** is a lock: one task takes it, uses the resource, and releases it. Others wait.

**Why a mutex here and not a queue?** A queue moves *data*. A mutex protects *access to a device*. Rule of thumb: data between tasks → queue; one physical device used by several tasks → mutex, or better, give the device to exactly one task (we will do exactly that in Week 13).

Two professional rules:
1. Always take a mutex with a timeout, and handle the failure. A task that waits forever turns one bug into a frozen system.
2. FreeRTOS mutexes include "priority inheritance": if a low-priority task holds the lock, it is temporarily boosted so a high-priority task is not stuck behind it. This exact problem (called priority inversion) once caused repeated restarts of the Mars Pathfinder spacecraft. It is worth knowing the name.

**Do.** First, make two tasks print long lines with no lock, and look at the mixed output. Then fix it:

```cpp
SemaphoreHandle_t mtx;   // create with: mtx = xSemaphoreCreateMutex();

void safePrint(const char *s) {
    if (xSemaphoreTake(mtx, pdMS_TO_TICKS(1000)) == pdTRUE) {
        Serial.println(s);
        xSemaphoreGive(mtx);
    } else {
        // could not get the lock within 1 second — record this, never ignore it
    }
}
```

**Check:** you saw the mixed output, fixed it, and can define priority inversion in one sentence.

### Day 16 🔧 — Interrupts, done correctly
**Goal:** learn the standard pattern: the interrupt signals, a task does the work.

**Concept.** An interrupt handler (ISR) interrupts everything, even the operating system itself. Because of this, an ISR must follow strict rules: keep it tiny, do not allocate memory, do not print, use only functions whose name ends in `FromISR`, and mark it `IRAM_ATTR` (Day 9). The professional pattern is: the ISR only gives a signal (a semaphore), and a normal task wakes up and does the real work.

**Why this pattern and not "do the work in the ISR"?** Work inside an ISR blocks the whole system, breaks timing for every task, and most library functions are simply not allowed there. The signal-then-work pattern keeps the ISR under a microsecond and moves the logic to a place where all tools are available.

**Do.** Connect a button (or a wire to GND) on pin 27. This simulates a rain sensor.

```cpp
SemaphoreHandle_t rainSem;

void IRAM_ATTR rainIsr() {
    BaseType_t woke = pdFALSE;
    xSemaphoreGiveFromISR(rainSem, &woke);   // only signal, nothing else
    if (woke) portYIELD_FROM_ISR();          // switch to the waiting task now
}
void rainTask(void*) {
    for (;;) {
        xSemaphoreTake(rainSem, portMAX_DELAY);   // sleeps until the ISR signals
        Serial.println("Rain detected. Cancel watering.");
        vTaskDelay(pdMS_TO_TICKS(200));           // simple debounce pause
    }
}
void setup() {
    Serial.begin(115200);
    rainSem = xSemaphoreCreateBinary();
    pinMode(27, INPUT_PULLUP);
    attachInterrupt(27, rainIsr, FALLING);
    xTaskCreatePinnedToCore(rainTask, "rain", 4096, NULL, 2, NULL, 1);
}
void loop() {}
```

Then break it on purpose: put `Serial.println` *inside* the ISR. The chip will crash. Read the decoded crash report (the monitor filter from Day 2 makes it readable).

**Check:** the pattern works, and you read and understood the crash report.

### Day 17 🔧 — Stack sizes and the watchdog, from measurement
**Goal:** stop guessing stack sizes; meet the watchdog on your own terms.

**Concept.** Every task has its own stack (the number 4096 in `xTaskCreatePinnedToCore` — it is in bytes). Too small → memory corruption and strange crashes. Too big → wasted RAM. Do not guess. The function `uxTaskGetStackHighWaterMark(NULL)` reports the *minimum free stack the task has ever had*. Measure it during development, then set the size to the worst case plus about 50 % reserve.

**Do.** Print the high-water mark from your Day 13 tasks every few seconds. Then trigger the watchdog deliberately: create a priority-5 task with an endless empty loop `for(;;){}` (it never blocks). The task watchdog will restart the chip. On reboot, check `esp_reset_reason()`.

**Check:** real stack numbers are in your log, and you can explain why the watchdog restart is a feature, not a bug.

### Day 18 💻🔧 — Self-test + a small project
**Self-test (no looking):**
1. In a data race, at which exact step is an increment lost?
2. Why does waiting on a queue cost no CPU?
3. Why do ISRs need special `FromISR` functions?
4. What problem does priority inheritance prevent?
5. How do you choose a stack size professionally?

**Small project** (two sessions are allowed): type a number in the serial monitor → the LED blink period changes to that number of milliseconds. Structure: one task reads the serial port → sends the number through a queue → a second task blinks. No shared variables at all. Check your own code for this.

**Check:** self-test passed; project works with zero shared variables.

---

# WEEK 5 — Measuring the soil
**Goal of the week:** a soil moisture value you can trust: calibrated, filtered, and powered only when needed.

### Day 19 💻 — How the ADC works, and where it is weak
**Goal:** understand the measurement chain, so the numbers mean something.

**Concept.** The ESP32 ADC converts a voltage to a 12-bit number (0–4095). Facts you need:
1. The ADC itself measures only 0 to about 1.1 V. An internal **attenuator** (a signal divider) extends the range. The setting `ADC_11db` gives roughly 0–3.3 V, but the result is accurate only between about 0.15 V and 2.45 V. Near the edges, the scale is compressed.
2. The conversion curve is not a straight line, and it differs from chip to chip. The factory stores a per-chip correction inside the chip. The function `analogReadMilliVolts(pin)` applies this correction and returns millivolts.
   **Why `analogReadMilliVolts` and not raw `analogRead`?** Raw counts have no defined meaning without the attenuation setting and the per-chip correction. Millivolts are a real physical unit, comparable between boards. Use raw counts only if you have a special reason.
3. The pins in the ADC2 group stop measuring while WiFi is on (Day 8). Sensors go on ADC1 pins (32–39).
4. The ADC is noisy. We will filter with the **median** (the middle value of a sorted list), not the average. Reason: a single large noise spike shifts the average, but the median simply ignores it.

**Do.** Answer in your log: "raw count 2048 equals what voltage?" Explain why this question has no answer without more information, and what information is missing.

**Check:** you can justify, in one short paragraph, why `analogReadMilliVolts` exists.

### Day 20 🔧 — See the noise, then filter it
**Do.**

```cpp
#include <algorithm>
void setup() {
    Serial.begin(115200); delay(300);
    analogSetPinAttenuation(34, ADC_11db);
    uint16_t v[100]; uint32_t sum = 0;
    for (int i = 0; i < 100; i++) { v[i] = analogReadMilliVolts(34); sum += v[i]; delay(2); }
    std::sort(v, v + 100);
    Serial.printf("min %u  max %u  average %lu  median %u\n", v[0], v[99], sum/100, v[50]);
}
void loop() {}
```

Run it twice: once normally, once while holding the sensor wire in your hand. Your body works as an antenna and injects noise. Compare how the average moves and how the median stays stable.

**Check:** your numbers show the median rejecting the noise that the average absorbs.

### Day 21 🔧 — The sensor driver
**Goal:** a clean, reusable sensor module in `lib/soil/`.

**Concept.** How a capacitive soil sensor works: the probe is a capacitor printed on the circuit board. Water changes the capacitance strongly (the dielectric constant of water is about 80; of dry soil about 4). A small oscillator on the sensor converts capacitance to a DC voltage. Two consequences:
1. The output is *inverted*: wetter soil gives a *lower* voltage.
2. The output is uncalibrated: "dry" and "wet" voltages differ per sensor, per soil, and per insertion depth. You must calibrate it in place (Day 22).

**Why a capacitive sensor and not a resistive one?** A resistive sensor passes direct current through the soil. This causes electrolysis, and the metal probe corrodes away within weeks. A capacitive sensor has no exposed metal contact with the soil and lasts years. There is no real trade-off here; resistive probes are simply the wrong tool.

**Why power the sensor from a GPIO pin?** The sensor draws about 5 mA all the time. On a battery device, we switch it on only for the measurement (a fraction of a second every 10 minutes). A GPIO pin can supply 5 mA directly, so no extra parts are needed.

**Do.** Create `lib/soil/soil.h`. Wire the sensor: signal → pin 34, power → pin 25.

```cpp
#pragma once
#include <Arduino.h>
#include <algorithm>

class SoilSensor {
public:
    SoilSensor(int adcPin, int powerPin) : adc_(adcPin), pwr_(powerPin) {}

    void begin() {
        pinMode(pwr_, OUTPUT); digitalWrite(pwr_, LOW);
        analogSetPinAttenuation(adc_, ADC_11db);
    }

    // Median of n readings. Sensor is powered only during the measurement.
    uint16_t readMilliVolts(int n = 9) {
        digitalWrite(pwr_, HIGH); delay(50);      // let the oscillator stabilize
        uint16_t v[15]; n = std::min(n, 15);
        for (int i = 0; i < n; i++) { v[i] = analogReadMilliVolts(adc_); delay(3); }
        digitalWrite(pwr_, LOW);
        std::sort(v, v + n);
        return v[n / 2];                          // the median
    }

    // Convert to 0–100 %. Note: dryMv is HIGHER than wetMv (inverted sensor).
    uint8_t percent(uint16_t mv, uint16_t dryMv, uint16_t wetMv) {
        mv = constrain(mv, std::min(dryMv, wetMv), std::max(dryMv, wetMv));
        return (uint8_t)(100L * (dryMv - mv) / (dryMv - wetMv));
    }
private:
    int adc_, pwr_;
};
```

**Check:** readings are stable, and you verified with the multimeter that the power pin is LOW between measurements.

### Day 22 🔧 — Calibrate the sensor
**Goal:** your sensor's personal "dry" and "wet" values.

**Why calibration values must not be written into the code:** in a real product, every unit needs different values, and flashing a different firmware for each unit is not workable. The values belong in the settings storage (Week 6). Today, only measure and write them down.

**Do.**
1. Sensor in dry air: take 5 readings, note the median. This is **dryMv**.
2. Sensor in a glass of water, up to the marked line: 5 readings, note the median. This is **wetMv**.
3. Sanity test: measure a damp sponge and compute `percent()`. It should land somewhere in the middle.

**Check:** dryMv and wetMv are in your log; the sponge result is roughly 40–70 %.

### Day 23 🔧 — Break it + self-test
**Do.** Move the sensor signal temporarily to pin 25 (an ADC2 pin), add `WiFi.begin("x","y");` to setup, and watch the readings fail while WiFi is active. Move it back to pin 34.

**Self-test:**
1. Why millivolts instead of raw counts?
2. Why must the sensor be on an ADC1 pin?
3. Why median and not average?
4. Why does a capacitive sensor read *lower* voltage when the soil is wet?
5. Why do calibration values belong in storage, not in the code?

**Check:** failure observed and explained; five answers written.

---

# WEEK 6 — Settings that survive restarts and reflashing
**Goal of the week:** calibration and settings stored safely in flash, and you understand why it works.

### Day 24 💻 — Why the ESP32 has no EEPROM, and what replaces it
**Goal:** understand flash memory physics, so the storage rules make sense.

**Concept.** On AVR you had EEPROM: write any single byte, any time, about 100,000 times per byte. The ESP32 has only flash memory, and flash has hard physical rules:
1. Writing can only change bits from 1 to 0. To change a bit back to 1, the chip must **erase a whole 4 KB block**. An erase takes 10–100 ms and briefly freezes the program (Day 3: the program runs from flash through a cache).
2. Each block survives about 100,000 erase cycles. After that, it dies.

If you emulate EEPROM naively (erase the same block for every small write), you destroy the flash quickly. Do the math: one settings write every 10 seconds → 100,000 erases are used in about 11.6 days.

**NVS** is Espressif's solution. It is a key-value database in a dedicated flash partition. Instead of overwriting, it *appends* each new value and remembers which entry is the newest. Erases are spread over many blocks ("wear leveling"). You get safe settings storage without thinking about flash physics.

**Why NVS and not writing our own file or raw flash records?** NVS already solves wear leveling, power-loss safety (a write interrupted by power failure does not corrupt old data), and versioned keys. Rebuilding that correctly costs weeks and is exactly the kind of code that fails rarely and terribly.

**Do.** Write the 11.6-day calculation in your log, step by step.

**Check:** the calculation is in the log.

### Day 25 💻🔧 — The Config module
**Goal:** all settings of the project in one struct, stored in NVS.

**Design rule:** *defaults in the code, real values in NVS.* A brand-new chip works immediately with defaults. A configured chip keeps its personal values even when you flash a new firmware. Reason: NVS lives in its own partition (the Day 3 map), and flashing the program does not touch that partition.

**Do.** Create `lib/config/config.h`:

```cpp
#pragma once
#include <Preferences.h>

struct Config {
    uint16_t soilDryMv      = 2800;   // replace defaults with YOUR Day 22 values
    uint16_t soilWetMv      = 1300;
    uint8_t  moistureLowPct = 35;     // water the plants below this percentage
    uint16_t waterSeconds   = 180;    // one watering dose
    uint32_t sleepSeconds   = 600;    // pause between measurements
    uint8_t  nodeId         = 2;      // the base station is number 1

    void load() {
        Preferences p;
        p.begin("cfg", true);                        // namespace "cfg", read-only mode
        soilDryMv      = p.getUShort("dryMv",  soilDryMv);
        soilWetMv      = p.getUShort("wetMv",  soilWetMv);
        moistureLowPct = p.getUChar ("lowPct", moistureLowPct);
        waterSeconds   = p.getUShort("wSec",   waterSeconds);
        sleepSeconds   = p.getUInt  ("slpSec", sleepSeconds);
        nodeId         = p.getUChar ("nodeId", nodeId);
        p.end();
    }
    void save() {
        Preferences p;
        p.begin("cfg", false);
        p.putUShort("dryMv", soilDryMv);   p.putUShort("wetMv", soilWetMv);
        p.putUChar ("lowPct", moistureLowPct);
        p.putUShort("wSec", waterSeconds); p.putUInt("slpSec", sleepSeconds);
        p.putUChar ("nodeId", nodeId);
        p.end();
    }
};
```

Add a serial command `dump` that prints all fields.

**Check:** compiles in both environments; `dump` shows the defaults.

### Day 26 🔧 — Prove that settings survive
**Do.**
1. Put your Day 22 calibration values into the struct and call `save()`.
2. Restart the board. Call `load()`. The values are still there.
3. Now flash the complete firmware again. The values are *still* there.
4. Explain in the log, using the partition map, which region of flash survived the reflash and why.

**Check:** calibration survived a full reflash, and the explanation is written.

### Day 27 🔧 — Storage timing + self-test
**Do.** Measure the time of 1000 `putUInt` calls to the same key (use `millis()` around the loop). Print `p.freeEntries()` before and after. You will see: most writes are fast, but sometimes one write takes much longer. That is NVS cleaning up a full page. Lesson: never put a settings write inside time-critical code.

**Self-test:**
1. Why does naive EEPROM emulation destroy flash?
2. What does wear leveling do?
3. Why do settings survive a firmware reflash?

**Check:** the slow-write event observed; three answers written.

---

# WEEKS 7–8 — The LoRa radio and the E32 module
**Goal of the fortnight:** two boards exchange addressed messages, and you have seen how wrong settings fail silently.

### Day 28 💻 — How LoRa reaches kilometers with milliwatts
**Goal:** understand the physics, because it dictates your design limits.

**Concept.** LoRa sends each symbol as a "chirp": a signal that sweeps across the whole channel bandwidth. The receiver knows the chirp shape and can find it by correlation even when the signal is *weaker than the background noise*. This is the trick behind the long range.

The price of this trick is **time**. The main setting is the spreading factor (SF7 to SF12). Each step up doubles the time per symbol, adds about 2.5 dB of range budget, and halves the data rate. The E32 hides this setting behind one simpler parameter called **air data rate** (0.3 to 19.2 kbit/s). Low air data rate = long range = long transmission time.

Two practical facts:
1. **Antenna height beats transmitter power.** Obstacles near the ground (soil, walls, wet leaves — a plant is mostly water) absorb the signal. Raising the antenna often helps more than any setting.
2. **The law limits your airtime.** Sub-GHz bands have duty-cycle rules. In the EU on 868 MHz, a device may typically transmit only 1 % of the time (36 seconds per hour). Check the rule for your country. This is a design input: it forces short, rare messages.

**Do.** Explain in your log, in your own words: why can LoRa decode a signal below the noise level, and what does this ability cost?

**Check:** you wrote "robustness is paid with airtime" as your own conclusion, with the reasoning.

### Day 29 💻 — Your airtime budget
**Do.** Calculate, with your country's rule:
1. Airtime of one 24-byte packet at 2.4 kbit/s: (24 × 8) / 2400 ≈ 80 ms, plus preamble and overhead → realistically 150–250 ms.
2. The legal maximum number of packets per hour.
3. Your design rate: at most 10 % of the legal maximum. Reason for the reserve: retransmissions also cost airtime, and you want a large safety margin against the law.

**Why 2.4 kbit/s and not slower or faster?** Slower (0.3 kbit/s) gives more range but each packet takes around ten times longer, which eats the legal budget and the battery. Faster (19.2 kbit/s) shortens packets but loses range. 2.4 kbit/s is a good middle point for a garden distance. If your range test (Day 69) fails, lowering this value is the second remedy, after raising the antenna.

**Check:** three numbers in the log: airtime, legal ceiling, your chosen sending period.

### Day 30 🔧 — Wire the first E32 (long day, plan a weekend)
**Goal:** the first module connected and alive.

**Concept.** The E32 module contains a LoRa radio chip plus a small controller with EBYTE's firmware. You talk to it over UART. Three extra pins control it:
- **M0 and M1** select the mode: 00 = normal work, 01 = transmit with wake-up preamble, 10 = power-saving receive, **11 = sleep and configuration**. In mode 11, the module always listens at 9600 baud for configuration commands.
- **AUX** is a "busy" output: LOW means the module is starting up, transmitting, or receiving. **Rule: wait until AUX is HIGH before changing modes or sending data.** Ignoring AUX is the most common reason for randomly lost packets.

Power detail: during transmission the module draws over 100 mA in short bursts. Without a capacitor next to the module, the 3.3 V line dips, and the ESP32 restarts (a brownout — Day 4). The 470 µF capacitor prevents this.

**Do.** Connect the antenna first. Then wire:

| E32 pin | Connect to | Note |
|---|---|---|
| VCC | 3.3 V | with the 470 µF capacitor directly at the module |
| GND | GND | common ground |
| TXD | GPIO 16 | module TX goes to ESP32 RX (crossed) |
| RXD | GPIO 17 | module RX comes from ESP32 TX |
| M0 | GPIO 21 | |
| M1 | GPIO 19 | |
| AUX | GPIO 18 | input |

**Check:** photo taken; after power-on, AUX goes HIGH (measure it or read it with `digitalRead`).

### Day 31 🔧 — Read the configuration and decode it yourself
**Goal:** understand the configuration bytes, so the library never surprises you.

**Concept.** In mode 11, the command `C0 ADDH ADDL SPED CHAN OPTION` writes a permanent configuration. The meaning of each byte:

| Byte | Meaning |
|---|---|
| ADDH, ADDL | 16-bit module address (0xFFFF = broadcast to all) |
| SPED | bits 7–6: UART parity; bits 5–3: UART speed; bits 2–0: air data rate |
| CHAN | frequency = band base + CHAN × 1 MHz |
| OPTION | bit 7: fixed (1) or transparent (0) transmission; bit 2: error correction on/off; bits 1–0: transmit power |

**Do.** Read the current configuration with the library, print the raw bytes, and decode them by hand using the table. Only then compare with the library's own interpretation.

```cpp
#include "LoRa_E32.h"
LoRa_E32 e32(&Serial2, 18 /*AUX*/, 21 /*M0*/, 19 /*M1*/);

void setup() {
    Serial.begin(115200); delay(300);
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    e32.begin();
    ResponseStructContainer c = e32.getConfiguration();
    Configuration cfg = *(Configuration*)c.data;
    Serial.printf("ADDR %02X%02X CHAN %02X SPED %02X fixed=%d fec=%d power=%d\n",
        cfg.ADDH, cfg.ADDL, cfg.CHAN, *(uint8_t*)&cfg.SPED,
        cfg.OPTION.fixedTransmission, cfg.OPTION.fec, cfg.OPTION.transmissionPower);
    c.close();
}
void loop() {}
```

**Check:** your manual decoding matches the library's fields.

### Day 32 🔧 — Wire and configure the second module
**Goal:** both modules configured the same way, with different addresses.

**Decision: fixed transmission, not transparent.** In *transparent* mode, every module with the same settings hears everything — the radio behaves like one shared serial cable. In *fixed* mode, you put the target's address in front of each message, and the module delivers it only there. We choose fixed, because our system has different devices with different roles, and addressing per device is exactly what your old nRF24 gave you with its "pipes". Transparent mode would force us to build addressing in software for no benefit.

**Do.** Wire board 2 the same way. Create `lib/radio/radio.h` and run it on both boards (base = address 0x0001, node = 0x0002; the channel and the air data rate must be identical on both):

```cpp
#pragma once
#include "LoRa_E32.h"
#define CHANNEL 23

inline LoRa_E32 e32(&Serial2, 18, 21, 19);   // AUX, M0, M1

inline void radioInit(uint16_t myAddr) {
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    e32.begin();
    ResponseStructContainer c = e32.getConfiguration();
    Configuration cfg = *(Configuration*)c.data;
    cfg.ADDH = myAddr >> 8;  cfg.ADDL = myAddr & 0xFF;
    cfg.CHAN = CHANNEL;
    cfg.OPTION.fixedTransmission = FT_FIXED_TRANSMISSION;
    cfg.OPTION.fec = FEC_1_ON;                     // error correction: on
    cfg.OPTION.transmissionPower = POWER_20;       // check your legal limit
    cfg.SPED.airDataRate = AIR_DATA_RATE_010_24;   // 2.4 kbit/s (Day 29 decision)
    cfg.SPED.uartBaudRate = UART_BPS_9600;
    e32.setConfiguration(cfg, WRITE_CFG_PWR_DWN_SAVE);   // store permanently
    c.close();
}
```

**Check:** after a power cycle, both modules still report the intended configuration.

### Day 33 🔧 — First contact
**Do.** For one day only, set both modules to transparent mode with the same address. Run this small bridge on both boards:

```cpp
void loop() {
    while (Serial.available())  Serial2.write(Serial.read());   // keyboard → radio
    while (Serial2.available()) Serial.write(Serial2.read());   // radio → screen
}
```

Type in one serial monitor, read in the other. Your first bytes over the air. Then notice what is missing: you do not know who sent a message, or for whom it was. That is why tomorrow we return to fixed mode.

**Check:** two-way chat works at 2 meters.

### Day 34 🔧 — Addressed delivery
**Do.** Restore the fixed configuration. Send from the node to the base:

```cpp
// node:  e32.sendFixedMessage(0x00, 0x01, CHANNEL, (uint8_t*)"ping", 4);
// base:  if (e32.available()) { ResponseContainer r = e32.receiveMessage();
//            Serial.println(r.data); }
```

Add a third test: send to a wrong address (0x0003). The base must NOT receive it.

**Check:** correct address delivers; wrong address verifiably does not.

### Day 35 🔧 — Silent radio failures, on purpose
**Goal:** see the three classic configuration mistakes now, in a controlled test.

**Do.**
1. Set different air data rates on the two boards. Result: complete silence, no error anywhere. Reason: chirps with different spreading factors are almost invisible to each other.
2. Same rates, different channels. Silence again.
3. Optional, one time: remove the 470 µF capacitor and transmit at full power from a weak USB port. The ESP32 restarts. Confirm with `esp_reset_reason()` that it was a brownout. Put the capacitor back.

**Check:** one log line per failure: symptom → cause.

### Day 36 💻🔧 — Self-test + first range test
**Self-test:**
1. Why does a lower air data rate give more range but longer airtime?
2. What do your Day 29 numbers mean for the sending period?
3. What does the AUX pin tell you, and when must you wait for it?
4. Fixed vs transparent: what is the difference, and why did we choose fixed?
5. Decode from memory: what does bit 7 of the OPTION byte control?

**Do.** A simple range test: the base at home prints a counter of received packets; the node runs from a USB power bank and sends numbered messages. Walk away until reception stops. Note the distance and the obstacles.

**Check:** self-test passed; a rough range number is in the log.

---

# WEEKS 9–10 — A reliable protocol
**Goal of the fortnight:** guaranteed delivery over an unreliable radio: checked with CRC, confirmed with ACK, repeated on failure, and tested automatically on your PC.

**Why we must build this at all:** your old nRF24L01 chip did acknowledgment, retries, and CRC in hardware, for free. The E32 does not. It delivers raw bytes: sometimes damaged, sometimes lost, sometimes twice. If we do nothing, the system will "mostly work", which is the worst possible state for a device that controls water.

### Day 37 💻 — The three ways a radio link fails
**Goal:** know the complete failure list and the cure for each one.

**Concept.** A radio link fails in exactly three ways. Each has a standard cure:

| Failure | How we detect it | How we fix it |
|---|---|---|
| **Corruption** — bits changed on the way | CRC check | throw the packet away |
| **Loss** — the packet never arrived | ACK with a timeout | send again (retry) |
| **Duplication** — a retry arrives although the original already arrived (its ACK was lost) | sequence numbers | design commands to be safe when repeated |

A protocol that handles all three is a complete protocol. (TCP, the protocol of the internet, is built from these same three ideas.)

**Why CRC and not a simple checksum (sum of all bytes)?** A sum cannot see two swapped bytes: the sum stays the same, but the message is wrong. A sum also misses errors that cancel each other. CRC is different mathematics: it treats the whole message as one long binary number and computes the remainder of a division. This construction provably catches all single-bit and double-bit errors, and all short error bursts. The cost is a few lines of code. For a link that can open a water valve, this is not optional.

**"Safe when repeated" (idempotent) commands:** a command must be phrased so that receiving it twice causes no harm. "Open the valve for 300 seconds, command number 17" is safe: the node sees number 17 twice and ignores the second copy. "Toggle the valve" is dangerous: the second copy inverts the state. Rule for this project: no toggle commands, ever.

**Do.** Reproduce the table from memory. Then write two sentences against simple checksums.

**Check:** table and reasoning written.

### Day 38 💻 — The CRC function and the packet structure
**Goal:** the wire format of your protocol, as plain C++ with no hardware.

**Do.** Create `lib/protocol/crc16.h`:

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>

// CRC16-CCITT, polynomial 0x1021. A standard, well-studied choice.
inline uint16_t crc16_ccitt(const uint8_t *d, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)d[i] << 8;
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}
```

And `lib/protocol/packet.h`:

```cpp
#pragma once
#include <stdint.h>
#include <string.h>
#include "crc16.h"

enum MsgType : uint8_t {
    MSG_TELEMETRY = 1,   // node → base: moisture, battery, status
    MSG_ACK       = 2,   // "received" confirmation
    MSG_CMD_WATER = 3,   // base → node: open the valve
    MSG_CMD_CFG   = 4,   // base → node: change settings
};

// "packed" removes padding bytes, so the memory layout of the struct
// IS the format on the wire. This is safe here because both ends are
// ESP32 chips (same byte order, same alignment rules). Between two
// DIFFERENT processor types, you would instead copy field by field.
struct __attribute__((packed)) Packet {
    uint8_t  magic;        // always 0xA5; used to find the packet start in a byte stream
    uint8_t  version;      // protocol version, for future changes
    uint8_t  type;         // MsgType
    uint8_t  nodeId;
    uint16_t seq;          // sequence number, grows by 1 per message
    uint8_t  payload[16];  // the actual data
    uint16_t crc;          // CRC over all bytes before this field

    void seal()        { crc = crc16_ccitt((uint8_t*)this, sizeof(Packet) - 2); }
    bool valid() const {
        return magic == 0xA5 &&
               crc == crc16_ccitt((const uint8_t*)this, sizeof(Packet) - 2);
    }
};
// If someone changes the struct, this line stops the build and reminds
// them: the wire format changed, the version must be increased.
static_assert(sizeof(Packet) == 24, "wire format changed — increase version!");
```

**Check:** both environments compile, and you can explain when "packed struct as wire format" would be the *wrong* choice.

### Day 39 💻 — Tests that run on your PC
**Goal:** run protocol tests on the desktop, in seconds, without any hardware.

**Why test on the PC and not on the board?** One flash-and-check cycle on the board costs about a minute. A test run on the PC costs about two seconds. When you develop logic (not hardware access), this speed difference changes how you work. It is also how commercial teams work: all logic that does not touch hardware is tested on the desktop. Notice that `crc16.h` and `packet.h` contain no hardware calls at all. That was a deliberate design decision, made exactly for this.

**Do.** Add to `platformio.ini`:

```ini
[env:native]
platform = native
build_flags = -std=gnu++17
```

Create `test/test_protocol/test_main.cpp`:

```cpp
#include <unity.h>
#include "packet.h"

void test_crc_detects_corruption() {
    Packet p{}; p.magic = 0xA5; p.type = MSG_TELEMETRY; p.seq = 42;
    p.seal();
    TEST_ASSERT_TRUE(p.valid());
    p.payload[3] ^= 0x01;                    // change a single bit
    TEST_ASSERT_FALSE(p.valid());
}
void test_size_is_wire_contract() { TEST_ASSERT_EQUAL(24, sizeof(Packet)); }

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_crc_detects_corruption);
    RUN_TEST(test_size_is_wire_contract);
    return UNITY_END();
}
```

Run: `pio test -e native`. Measure how long it takes.

**Check:** tests are green in about two seconds.

### Day 40 💻 — Prove the CRC promises
**Do.** Write a test that flips every single bit of a sealed packet (24 bytes × 8 = 192 tests). The CRC must catch all of them:

```cpp
void test_crc_catches_every_single_bit_flip() {
    Packet p{}; p.magic = 0xA5; p.version = 1; p.type = MSG_TELEMETRY; p.seq = 7;
    p.seal();
    for (size_t byte = 0; byte < sizeof(Packet); byte++)
        for (int bit = 0; bit < 8; bit++) {
            Packet c = p;
            ((uint8_t*)&c)[byte] ^= (1 << bit);
            TEST_ASSERT_FALSE(c.valid());
        }
}
```

Then implement a naive byte-sum "checksum" and write the test that defeats it: swap two payload bytes. The sum stays the same; the message is wrong. The CRC catches this case; the sum does not.

**Check:** both tests behave exactly as predicted. Now you know why CRC — from evidence, not from authority.

### Day 41 🔧 — Framing: turning a byte stream into packets
**Goal:** the receive function.

**Concept.** UART gives you a *stream of bytes*, not packets. A packet can arrive in two pieces, or with garbage before it. **Framing** means: collect bytes, find the start marker (our 0xA5 "magic" byte), and assemble complete structures. One honest limitation: if the value 0xA5 appears by chance inside noise, we assemble a wrong packet once — and the CRC rejects it, and the stream recovers by itself.

**Why this simple method and not a "perfect" framing protocol (like SLIP or COBS)?** Perfect framing needs byte escaping on both sides: more code, more airtime per packet. Our method is a few lines, and its only weakness is already covered by the CRC. Right cost, right benefit for this link.

**Do.** Add to `lib/radio/`:

```cpp
inline bool tryReceive(Packet &out) {
    static uint8_t buf[sizeof(Packet)];
    static size_t  have = 0;
    while (Serial2.available()) {
        uint8_t b = Serial2.read();
        if (have == 0 && b != 0xA5) continue;    // searching for the start marker
        buf[have++] = b;
        if (have == sizeof(Packet)) {
            have = 0;
            memcpy(&out, buf, sizeof(Packet));
            if (out.valid()) return true;         // CRC is the final judge
            // invalid: false start marker or damage — drop it, keep searching
        }
    }
    return false;
}
```

Test it with deliberately fragmented sends: send half a packet, wait, send the rest.

**Check:** a packet split into two parts still assembles; garbage before the marker is skipped.

### Day 42 🔧 — Sending with confirmation and retries
**Goal:** the core of the reliable link.

**Concept.** The plan: send → wait for an ACK with the same sequence number → if no ACK within the timeout, wait a moment and send again → after several failures, give up and *count* the failure. Two details deserve explanation:
1. **Random jitter in the retry pause.** Imagine two nodes transmit at the same moment and collide. If both retry after exactly the same pause, they collide again, forever. A random extra delay breaks this symmetry. (Ethernet networks solved the same problem the same way in the 1970s.)
2. **Statistics counters.** Every success, retry, and failure is counted. Later, these counters are your only view into a device standing in a garden.

**Do.**

```cpp
struct LinkStats { uint32_t txOk = 0, txFailed = 0, retries = 0; };
inline LinkStats stats;

inline void sendAck(const Packet &rx, uint16_t dest) {
    Packet a{}; a.magic = 0xA5; a.version = 1;
    a.type = MSG_ACK; a.nodeId = rx.nodeId; a.seq = rx.seq;   // confirm THIS seq
    a.seal();
    e32.sendFixedMessage(dest >> 8, dest & 0xFF, CHANNEL, (uint8_t*)&a, sizeof(a));
}

inline bool sendReliable(Packet &p, uint16_t dest,
                         uint8_t maxRetries = 3, uint32_t timeoutMs = 1500) {
    p.seal();
    for (uint8_t attempt = 0; attempt <= maxRetries; attempt++) {
        if (attempt) stats.retries++;
        e32.sendFixedMessage(dest >> 8, dest & 0xFF, CHANNEL, (uint8_t*)&p, sizeof(p));
        uint32_t t0 = millis();
        while (millis() - t0 < timeoutMs) {
            Packet rx;
            if (tryReceive(rx) && rx.type == MSG_ACK && rx.seq == p.seq) {
                stats.txOk++;
                return true;                     // delivery is now PROVEN
            }
            vTaskDelay(pdMS_TO_TICKS(10));       // let other tasks run while we wait
        }
        vTaskDelay(pdMS_TO_TICKS(200 * (attempt + 1)      // growing pause
                   + (esp_random() % 150)));              // plus random jitter
    }
    stats.txFailed++;
    return false;
}
```

For today, the base station side is a simple loop that sends an ACK for every valid packet. Run node → base at 2 meters. Retries should be near zero.

**Check:** `sendReliable` returns true; the counters change as expected.

### Day 43 🔧 — Break the link, watch the recovery
**Do.** During a running test, switch the base station off for about 3 seconds. (Never remove an antenna as a "failure test" — transmitting without an antenna can destroy the module.) Watch in the timestamped monitor: the retries, the growing pauses, the final failure, the counter.

**Check:** using only the log timestamps, you can tell the story of one failed delivery.

### Day 44 💻 — Timeouts by calculation, not by feeling
**Goal:** a timeout you can defend.

**Do.** Calculate the minimum reasonable timeout:
airtime of one packet (Day 29) × 2 (message + ACK) + module processing time (about 200–400 ms) + a little slack for task scheduling. Compare with the default 1500 ms. Set your value with about 2× margin, and write the calculation down.

Also write down this rule: **if you ever lower the air data rate for more range, the airtime grows, and this timeout must be recalculated.** A timeout shorter than the airtime is a bug that looks like "the radio is unreliable".

**Check:** your timeout is a computed number with visible work.

### Day 45 💻 — Commands that are safe against duplicates and delays
**Goal:** the command payloads, plus a fix of an earlier design mistake.

**Concept — an honest correction.** An earlier version of this project let the node check a command's expiry time ("do not execute after 14:05"). That design was wrong. Why: a deep-sleeping node **has no clock with real time**. It cannot check a wall-clock condition. The corrected design splits the work by who actually knows what:
- The **base station** has real time (from the internet). It checks expiry *before* delivering a queued command. An old command is thrown away at the base.
- The **node** knows only sequence numbers. It rejects duplicates by remembering the last executed command number.

General lesson: distrust any design where a component checks information it cannot actually possess.

**Do.** Add to `lib/protocol/`:

```cpp
struct __attribute__((packed)) WaterCmd { uint16_t seconds; };

// Node side. lastSeq will live in RTC memory (Week 11), so it survives sleep.
inline bool isDuplicate(uint16_t seq, uint16_t &lastSeq) {
    if (seq == lastSeq) return true;
    lastSeq = seq;
    return false;
}

// Base side.
struct PendingCmd {
    uint8_t  nodeId; uint8_t type; uint16_t seconds;
    time_t   queuedAt;              // when the user created the command
    uint32_t ttlSeconds;            // how long it stays valid
};
inline bool cmdExpired(const PendingCmd &c, time_t now) {
    return now - c.queuedAt > (time_t)c.ttlSeconds;
}
```

Write native tests: a duplicate sequence number is rejected; an expired command is refused; a fresh command passes.

**Check:** tests green, and you can explain in one sentence why "toggle valve" is forbidden in this project.

### Day 46 💻 — Self-test + freeze the protocol
**Self-test:**
1. Name the three failure modes and the cure for each.
2. Why must commands be safe when repeated? Who checks expiry, and why that side?
3. What problem does the random retry jitter prevent?
4. Show your timeout calculation.
5. Why do the protocol files compile on a PC, and why will this matter again later (hint: a future move from the Arduino framework to Espressif's professional framework)?

**Do.** Commit and create a git tag: `git tag v0-protocol`. From now on, the wire format changes only together with a version number increase.

**Check:** self-test passed; the tag exists.

---

# WEEK 11 — Deep sleep and the battery
**Goal of the week:** a *measured* sleep current and a battery-life prediction you can defend.

### Day 47 🔧 — Waking up is a reboot
**Goal:** understand what deep sleep destroys and what survives.

**Concept.** In deep sleep, both processor cores, all normal RAM, and all peripherals are switched off. Only the small RTC section survives: its timer, its 8 KB memory, and the wake-up logic. The chip alone then draws about 10 µA. Two consequences:
1. **Waking up runs `setup()` again, from zero.** All normal variables are gone. The firmware becomes a "run-to-completion" program: boot → do the work → sleep. There is no long-lived `loop()` anymore.
2. State that must survive goes to one of two places: **NVS** for settings (rarely changed), or **RTC memory** for small, frequently changing values like counters. RTC memory is used with the marker `RTC_DATA_ATTR`. It is fast and free, but it is lost when the battery is disconnected.

**Why deep sleep and not light sleep?** Light sleep keeps RAM alive and wakes faster, but draws about 0.8 mA — roughly 80 times more than deep sleep. Our node sleeps for 10 minutes at a time; the slower wake-up (a reboot, well under a second) costs us nothing, and the 80× battery saving decides the choice.

**Do.** Write your prediction first, then run this (30-second cycles):

```cpp
#include <Arduino.h>
RTC_DATA_ATTR uint32_t rtcCount = 0;   // lives in RTC memory
uint32_t ramCount = 0;                 // normal RAM

void setup() {
    Serial.begin(115200); delay(300);
    Serial.printf("wake cause=%d  rtc=%lu  ram=%lu\n",
        esp_sleep_get_wakeup_cause(), ++rtcCount, ++ramCount);
    esp_sleep_enable_timer_wakeup(30ULL * 1000000);   // value is in MICROseconds
    esp_deep_sleep_start();                            // this function never returns
}
void loop() {}
```

**Check:** rtcCount climbs, ramCount stays 1, and you can explain why without notes.

### Day 48 🔧 — Know why you woke up
**Do.** Extend Day 47:

```cpp
switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("periodic wake");           break;
    case ESP_SLEEP_WAKEUP_EXT0:  Serial.println("pin wake (e.g. sensor)");  break;
    default:                     Serial.println("power-on or reset — full initialization"); break;
}
// Optional pin wake-up (the pin must be in the RTC-capable group):
// esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);   // wake when the pin goes LOW
```

The difference matters: after a power-on you must do the full initialization (for example, configure the radio); after a timer wake, a faster path is possible.

**Check:** each wake path prints its own message.

### Day 49 🔧 — The most important measurement of the project (weekend, do not rush)
**Goal:** the real sleep current of the whole board.

**Concept.** The chip's 10 µA is meaningless if the rest of the board leaks. Reality check:
- The devkit's USB chip and voltage regulator leak between 0.1 and 10 mA, depending on the board model. **This, not your code, usually decides the battery life.**
- An E32 in normal mode idles at tens of milliamps. **It must be put to sleep too** (M0 = M1 = 1). Its sleep current is a few microamps.
- The soil sensor is already switched off by its GPIO power pin (Day 21).

**Do.** Put the multimeter in series with the power supply. Measure three numbers:
1. Active current (during a transmission).
2. Sleep current with the E32 left awake.
3. Sleep current after calling `e32.setMode(MODE_3_SLEEP);` before `esp_deep_sleep_start()`.

**Check:** three numbers in the log. Number 2 minus number 3 shows you why the E32 sleep line exists.

### Day 50 💻 — The battery calculation
**Do.** With YOUR measured numbers:

```
average_mA   = (active_mA × active_seconds + sleep_mA × sleep_seconds) / cycle_seconds
battery_h    = capacity_mAh / average_mA
```

Reference example (10-minute cycle, 2000 mAh battery): active 4 s × 80 mA = 320 mA·s; sleep 596 s × 0.15 mA = 89 mA·s; average ≈ 0.68 mA → about 2900 hours ≈ **4 months**. The same firmware on a leaky devkit with 5 mA sleep floor: about **2 weeks**. Conclusion: the sleep current dominates everything. That is why Day 49 measured instead of estimated.

Also add battery monitoring now: two 470 kΩ resistors as a divider from the battery to pin 35; in code, `battMv = analogReadMilliVolts(35) * 2`. (Large resistor values, so the divider itself wastes almost nothing.)

**Check:** a defensible battery-life number, plus the name of your biggest consumer and its fix (usually: a board with a better regulator).

### Day 51 💻 — How a command reaches a sleeping node + self-test
**Goal:** the "rendezvous" design.

**Concept.** The node sleeps 99 % of the time. The base cannot push a command to a device that is not listening. Three known solutions:
1. **Polling rendezvous — our choice.** On every wake, right after sending telemetry, the node listens for about 2 seconds. The base uses exactly this window to deliver a queued command. Cost: a command waits at most one sleep period (10 minutes). For watering, this delay is acceptable.
2. *Wake-on-preamble.* The E32 supports a mode where the base sends a long preamble and sleeping receivers periodically sniff for it. Faster delivery, but more configuration complexity and more failure modes. A good later optimization, not a good starting point.
3. *Scheduled listening windows using synchronized clocks.* Rejected immediately: the node has no reliable clock (Day 45 taught us this lesson already).

The elegant detail of solution 1: **the arrival of telemetry is itself the proof that the node is awake right now.** The base does not guess; it reacts.

**Do.** Draw the message sequence: node wakes → sends telemetry → base ACKs → base immediately sends the queued command into the node's window → node ACKs the command → executes → sleeps.

**Self-test:**
1. Why do normal variables die across sleep, and `RTC_DATA_ATTR` variables survive?
2. Why is the E32's sleep more important than the ESP32's?
3. Reproduce the battery formula and your numbers.
4. Why does command delay equal the sleep period?

**Check:** the diagram is drawn from understanding; four answers written.

---

# WEEK 12 — The field node, complete
**Goal of the week:** the full cycle — wake, measure, decide, transmit, listen, sleep — on real hardware, with the valve and with layered safety.

### Day 52 💻 — Design before code
**Goal:** two design decisions, understood before they are typed.

**Decision 1 — local autonomy.** The node can water the plants on its own, using the threshold stored in its NVS settings, even if the base station is dead. **Why:** the radio link makes the system *better*, but it must never be a single point of failure for plant survival. If we required a working base for every watering, one router problem during your vacation would kill the garden.

**Decision 2 — layered safety for the valve.** Three independent layers, each catching a failure the others cannot:
1. A hard software limit on watering duration (catches a wrong or malicious command value).
2. Expiry at the base plus duplicate rejection at the node (catches delayed and repeated commands — Day 45).
3. "Safe state first": the very first lines of `setup()` force the valve pin LOW, and the watchdog restarts a frozen firmware. Together they catch the case that scares us most: the program hangs *while the valve is open*. Layers 1 and 2 cannot help there; layer 3 can.

**Do.** Draw the state machine of the node. Next to each safety layer, write the one failure that only this layer catches.

**Check:** you can defend "the base may die; the plants must not" in two sentences.

### Day 53 💻🔧 — Assemble the main program
**Do.** `src/field_node/main.cpp`. Until Day 54, let an LED play the role of the valve.

```cpp
#include <Arduino.h>
#include "config.h"
#include "soil.h"
#include "radio.h"
#include "packet.h"

#define PIN_VALVE 26
#define PIN_BATT  35
#define BASE_ADDR 0x0001

Config cfg;
SoilSensor soil(34, 25);
RTC_DATA_ATTR uint16_t txSeq = 0;        // survives sleep (Day 47)
RTC_DATA_ATTR uint16_t lastCmdSeq = 0;   // for duplicate rejection (Day 45)
RTC_DATA_ATTR uint16_t bootCount = 0;

// The very first action after ANY reset: valve closed. Safety layer 3.
static void safeState() {
    pinMode(PIN_VALVE, OUTPUT);
    digitalWrite(PIN_VALVE, LOW);
}

static void waterFor(uint16_t seconds) {
    seconds = min(seconds, (uint16_t)600);   // hard limit. Safety layer 1.
    digitalWrite(PIN_VALVE, HIGH);
    for (uint16_t s = 0; s < seconds; s++) vTaskDelay(pdMS_TO_TICKS(1000));
    digitalWrite(PIN_VALVE, LOW);
}

static void goToSleep() {
    e32.setMode(MODE_3_SLEEP);               // the E32 must sleep too (Day 49)
    esp_sleep_enable_timer_wakeup((uint64_t)cfg.sleepSeconds * 1000000ULL);
    esp_deep_sleep_start();
}

void setup() {
    safeState();
    Serial.begin(115200);
    bootCount++;
    cfg.load();
    soil.begin();
    radioInit(cfg.nodeId);

    // 1. Measure.
    uint16_t mv   = soil.readMilliVolts();
    uint8_t  pct  = soil.percent(mv, cfg.soilDryMv, cfg.soilWetMv);
    uint16_t batt = analogReadMilliVolts(PIN_BATT) * 2;

    // 2. Local decision — works even when the base is dead (Day 52, decision 1).
    bool watered = false;
    if (pct < cfg.moistureLowPct && batt > 3500) {   // low battery blocks watering
        waterFor(cfg.waterSeconds);
        watered = true;
    }

    // 3. Send telemetry, with confirmation.
    Packet p{}; p.magic = 0xA5; p.version = 1;
    p.type = MSG_TELEMETRY; p.nodeId = cfg.nodeId; p.seq = ++txSeq;
    p.payload[0] = pct;
    memcpy(&p.payload[1], &batt, 2);
    p.payload[3] = watered;
    sendReliable(p, BASE_ADDR);

    // 4. The rendezvous window: listen 2 seconds for a queued command (Day 51).
    uint32_t t0 = millis();
    while (millis() - t0 < 2000) {
        Packet rx;
        if (tryReceive(rx) && rx.nodeId == cfg.nodeId) {
            sendAck(rx, BASE_ADDR);                       // confirm first
            if (!isDuplicate(rx.seq, lastCmdSeq)) {       // safety layer 2, node half
                if (rx.type == MSG_CMD_WATER) {
                    uint16_t sec; memcpy(&sec, &rx.payload[0], 2);
                    waterFor(sec);
                } else if (rx.type == MSG_CMD_CFG) {
                    applyConfig(rx.payload, cfg);         // your homework, see below
                    cfg.save();
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    // 5. Sleep.
    goToSleep();
}
void loop() {}   // never reached: setup() ends in deep sleep
```

**Homework:** write `applyConfig` yourself. It copies fields from the payload into `cfg`. It is left to you on purpose: writing it forces you to define and own the payload layout, instead of renting mine.

**Check:** the full cycle runs with the LED; telemetry arrives at the base.

### Day 54 🔧⚡ — The valve circuit (weekend, calm hands)
**Goal:** the switching circuit, with a reason for every part.

**Concept.** A solenoid valve is a coil, which means an inductor. An inductor resists changes of its current. When you switch it off, the collapsing magnetic field raises the voltage sharply until the current finds a path. Without protection, that path is *through your transistor*, which destroys it. The protection is a diode across the coil: it gives the current a harmless loop.

The parts, and why each exists:
- **Logic-level N-MOSFET (IRLZ44N).** "Logic-level" means it switches fully on with only 3.3 V at the gate. **Why not the common IRFZ44N?** It needs ~10 V at the gate; at 3.3 V it only half-opens, acts as a resistor, and overheats.
- **Why a MOSFET and not a relay?** A relay works, but it clicks, wears out mechanically, and its coil consumes constant current while on — bad for a battery. The MOSFET is silent, wear-free, and wastes almost nothing.
- **100 Ω resistor** between GPIO and gate: limits the brief charging current of the gate, protecting the GPIO pin.
- **100 kΩ resistor** from gate to GND: during boot, GPIO pins float (undefined). This resistor holds the valve firmly OFF until the firmware takes control. It is a safety part, not decoration.
- **Flyback diode (1N5819) across the valve coil, cathode to +12 V:** the discharge path described above.
- The 12 V supply and the ESP32 share **ground only**. Never connect 12 V to anything on the ESP32 side.

Connections: GPIO 26 → 100 Ω → gate. Source → GND. Valve between +12 V and drain. Diode across the valve. 100 kΩ from gate to GND.

**Battery note for later:** a normal solenoid *holds* about 500 mA the whole time it is open. Three minutes of watering ≈ 25 mAh — often more than the node uses in a full day. The professional answer is a **latching valve**: it needs only a short pulse to open or close and holds its state with zero current. Build the simple version now; write "latching valve" on your upgrade list.

**Do.** Build slowly. Check the diode direction three times before applying power.

**Check:** GPIO HIGH opens the valve. Power-cycling the ESP32 never makes the valve twitch — that is the 100 kΩ resistor doing its job.

### Day 55 🔧⚡ — Attack your own safety layers
**Do.** Three deliberate attacks:
1. Send a command for 10,000 seconds of watering → the software limit cuts it to 600.
2. Deliver the same command sequence number twice → the duplicate check refuses the second one.
3. Press reset *while watering* → the valve closes immediately, because `safeState()` is the first thing that runs.

**Check:** all three defenses hold, with timestamps in the log.

### Day 56 🔧 — The full cycle with real sleep
**Do.** Enable `goToSleep()` and run five complete cycles. Verify three things:
1. Sequence numbers continue across sleeps (RTC memory works).
2. Delivery is 5 of 5.
3. Sleep current still equals the Day 49 value. (Regressions creep in — forgetting the E32 sleep line after a code change is the classic one.)

**Check:** 5 of 5 clean cycles. The node is now a device, not a sketch.

### Day 57 🔧 — Rendezvous, live
**Do.** Put one hardcoded pending command into the base. Watch the full chain: node wakes → telemetry → ACK plus the command inside the 2-second window → node confirms → waters → sleeps. Measure the real delay from "command queued" to "valve open" and compare it with your sleep period.

**Check:** the round trip works twice in a row.

### Day 58 🔧 — Overnight test + self-test
**Do.** Run the system overnight with `sleepSeconds = 60` (more cycles = more chances for rare bugs to appear). In the morning: delivery percentage, retry counts, any unexplained restarts.

**Self-test:**
1. Why is `safeState()` the first call?
2. Name the three safety layers and the unique failure each one catches.
3. Why does the 100 kΩ resistor exist?
4. Where does the coil current flow after switch-off, and through which part?
5. Why is a latching valve better for a battery device?

**Check:** at least 99 % delivery overnight, or a written explanation of why not; five answers written.

---

# WEEK 13 — The base station: LoRa to MQTT bridge
**Goal of the week:** soil data on your phone; a tap on the phone waters a plant.

### Day 59 💻 — MQTT: what it is and why we use it
**Goal:** the concepts, plus a running broker.

**Concept.** MQTT is a small network protocol built around a central server called a **broker**. Devices **publish** messages to named channels called **topics** (example: `garden/node2/telemetry`), and other devices **subscribe** to topic patterns (example: `garden/+/command`). Publisher and subscriber never know about each other; the broker connects them.

**Why MQTT and not our own web server on the ESP32?** Three reasons:
1. *Decoupling.* Because publishers and subscribers are independent, you can later add a dashboard, a data logger, or a phone app without changing one line of firmware. A built-in web server ties the interface to the firmware forever.
2. *Less code.* A web server on the ESP32 means HTML, connections, and security handled by you. With MQTT, ready-made apps and Home Assistant do the interface work.
3. *It is the industry standard* for exactly this kind of device, so learning it has commercial value in itself.

Three MQTT features map directly onto your project:
- **QoS (quality of service) levels** repeat Week 9 exactly: QoS 0 = send and forget; QoS 1 = ACK and retry (duplicates possible — idempotency again); QoS 2 = exactly once. You already understand MQTT because you built its problems yourself. That was intentional.
- **Retained messages:** the broker stores the last message of a topic. A dashboard that connects later still sees the latest soil value.
- **LWT ("last will"):** a message the broker publishes automatically if a device disappears without saying goodbye. The broker reports the death of your base station for you.

**Do.** Install the Mosquitto broker (on a PC, Raspberry Pi, or in Docker). Play with it:

```
mosquitto_sub -t "garden/#" -v
mosquitto_pub -t garden/test -m "hello" -r     # -r stores it as retained
# disconnect and reconnect the subscriber: the retained message arrives again
```

**Check:** you can match every QoS level to the Week-9 mechanism it repeats, and you saw a retained message survive a reconnect.

### Day 60 🔧 — The ESP32 joins the broker
**Do.** Start `src/base_station/main.cpp`:

```cpp
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient net;
PubSubClient mqtt(net);

void connectMqtt() {
    // The last argument is the LWT: if we die, the broker publishes "offline".
    if (mqtt.connect("garden-base", "garden/base/status", 1, true, "offline")) {
        mqtt.publish("garden/base/status", "online", true);
        mqtt.subscribe("garden/+/command");
    }
}
void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) delay(200);
    configTime(0, 0, "pool.ntp.org");     // real time — needed for command expiry (Day 45)
    mqtt.setServer(MQTT_HOST, 1883);
    connectMqtt();
}
void loop() { mqtt.loop(); delay(50); }
```

Test the LWT: pull the board's power and watch the topic `garden/base/status` change to "offline" after the timeout.

**Check:** the broker announced the death for you.

### Day 61 💻🔧 — Two tasks, two queues
**Goal:** the base station architecture. This is where Weeks 3–4 pay off.

**Concept.** Unlike the node, the base runs all day and handles two independent jobs at once: the radio and the network. The architecture rule: **each physical device is owned by exactly one task**, and tasks talk only through queues.

```
loraTask  (owns the E32 UART: receives, ACKs, delivers queued commands)
   │  telemetryQ (Packet)          ▲  cmdQ (PendingCmd)
   ▼                               │
mqttTask  (owns WiFi and MQTT: publishes data, receives commands, reconnects)
```

**Why one owner per device and not mutexes everywhere?** Both are correct; ownership is simpler. With a single owner, there is no lock to forget and no deadlock to create. Mutexes remain for the rare case where ownership is impossible.

The Day 14 policies apply word for word: the telemetry queue may drop when full (old soil data is worthless); the command queue must never drop silently.

**Do.** Implement the skeleton:

```cpp
QueueHandle_t telemetryQ;   // carries Packet
QueueHandle_t cmdQ;         // carries PendingCmd (Day 45)

void loraTask(void*) {
    for (;;) {
        Packet rx;
        if (tryReceive(rx) && rx.type == MSG_TELEMETRY) {
            sendAck(rx, rx.nodeId);              // the node is awake RIGHT NOW —
            PendingCmd c;                        // this is the rendezvous moment (Day 51)
            if (peekCmdFor(rx.nodeId, c) && !cmdExpired(c, time(nullptr)))
                deliverCmd(c, rx.nodeId);        // uses sendReliable inside
            xQueueSend(telemetryQ, &rx, 0);      // full → drop. Policy from Day 14.
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

**Homework:** write `peekCmdFor` and `deliverCmd` — a small array of pending commands, searched by node number. About 20 lines. This is the last piece of glue code left to you.

**Check:** both tasks run, and a review of your code confirms: one owner per device, everywhere.

### Day 62 🔧 — Soil data on your phone
**Do.** In `mqttTask`, empty the telemetry queue into retained JSON messages:

```cpp
Packet p;
while (xQueueReceive(telemetryQ, &p, 0) == pdTRUE) {
    char topic[48], json[96];
    uint16_t batt; memcpy(&batt, &p.payload[1], 2);
    snprintf(topic, sizeof topic, "garden/node%u/telemetry", p.nodeId);
    snprintf(json, sizeof json,
        "{\"moisture\":%u,\"batt_mv\":%u,\"watered\":%u,\"seq\":%u}",
        p.payload[0], batt, p.payload[3], p.seq);
    mqtt.publish(topic, json, true);             // true = retained
}
```

View it in an MQTT phone app (for example "IoT MQTT Panel") or MQTT Explorer on the PC.

**Check:** fresh data appears within one node wake period, and after an app reconnect the last values are still shown (retained works).

### Day 63 🔧 — A tap on the phone waters a plant
**Do.** Parse incoming commands. We use a simple text format instead of JSON. **Why:** a JSON parser is an extra library and extra failure modes, and our command is one number. `water:30` is enough, and every MQTT app can send it.

```cpp
void onMqtt(char *topic, byte *payload, unsigned int len) {
    PendingCmd c{};
    if (sscanf(topic, "garden/node%hhu/command", &c.nodeId) != 1) return;
    char body[32] = {0};
    memcpy(body, payload, min(len, (unsigned)31));
    unsigned sec = 0;
    if (sscanf(body, "water:%u", &sec) == 1) {
        c.type = MSG_CMD_WATER;
        c.seconds = min(sec, 600u);              // limit at the base TOO — layered safety
        c.queuedAt = time(nullptr); c.ttlSeconds = 3600;
        xQueueSend(cmdQ, &c, portMAX_DELAY);     // commands never drop (Day 14)
    }
}
// in setup(): mqtt.setCallback(onMqtt);
```

From the phone, publish to topic `garden/node2/command` the message `water:30`. Watch the whole chain: broker → base queue → the node's 2-second window → valve open for 30 seconds → confirmation telemetry back.

**Check:** you watered a plant from your phone, through a system where you built and understand every layer.

### Day 64 🔧 — Kill the router, watch the self-healing
**Do.** Switch your WiFi router off for 5 minutes while everything runs. Find what breaks. The two classic holes: the code never re-checks `WiFi.status()`, and the MQTT reconnect forgets to **subscribe again** (subscriptions die with the connection). Fix with a guard at the top of the `mqttTask` loop:

```cpp
if (WiFi.status() != WL_CONNECTED) { WiFi.reconnect(); vTaskDelay(pdMS_TO_TICKS(2000)); continue; }
if (!mqtt.connected()) { connectMqtt(); }   // connectMqtt() also re-subscribes — that is why it is one function
```

Repeat the test until it is boring.

**Self-test:**
1. What does the publisher/subscriber decoupling buy you, concretely?
2. Match QoS 1 to your own Week-9 mechanisms.
3. Explain "retained" and "LWT", each with one garden example.
4. Why one task per device?
5. Explain, start to finish, how a command reaches a sleeping node.

**Check:** two consecutive router-kill cycles heal completely by themselves; five answers written.

### Day 65 💻 — Optional: Home Assistant
**Do.** If you use Home Assistant: add the MQTT integration, define sensors for moisture and battery (the retained topics fit directly), and a button that publishes `water:60`. If you do not use Home Assistant, the phone MQTT app is a complete solution; skipping this day is a valid choice.

**Check:** a moisture history graph exists — or the skip was a conscious decision.

---

# WEEKS 14–15 — Integration, deployment, observation
**Goal:** the system installed outside, reporting its own health, and watched for a full week.

### Day 66 💻 — The node reports its own health
**Concept.** The node stands in a garden. You cannot attach a serial cable. Therefore the telemetry *is* your log file. A device that reports its own vital signs is the difference between engineering and guessing.

**Do.** Every 6th wake, add a diagnostic payload (reuse MSG_TELEMETRY with a flag bit, or define MSG_DIAG): `esp_reset_reason()`, `bootCount`, `stats.txFailed`, `stats.retries`, and the stack high-water mark (Day 17). The base publishes it to `garden/nodeX/diag`, retained.

**Check:** diagnostic JSON is visible on the broker.

### Day 67 💻 — Two defensive behaviors
**Do.** Two small, hardware-free functions in `lib/protocol/`, with tests:

```cpp
// After repeated delivery failures, sleep longer. Purpose: if the base is
// dead for days, the node must not burn its battery on useless retries.
inline uint32_t nextSleepSeconds(uint32_t base, uint8_t consecFails) {
    uint8_t k = min<uint8_t>(consecFails / 3, 3);
    return base << k;                    // 1×, 2×, 4×, up to 8×
}

// Below this battery level, never water. Purpose: a chip that is browning
// out can restart repeatedly and flap the valve. Better: skip, sleep, report.
inline bool wateringAllowed(uint16_t battMv) { return battMv > 3500; }
```

Wire both into the field node (`consecFails` lives in `RTC_DATA_ATTR`).

**Check:** tests green; both behaviors integrated.

### Day 68 🔧 — The pre-deployment audit
**Do.** Formally, with results written down:
1. One hour of continuous operation at final settings → delivery at least 99 %.
2. Sleep current measured again → equals Day 49 within noise.
3. The three Day-55 safety attacks repeated → all held.

This log entry is your permission to deploy. Sign it.

**Check:** everything green, in writing.

### Day 69 🔧 — Range test at the real place (weekend)
**Do.** Node at its real spot, base at its real spot. Send 50 numbered packets; count arrivals. If the result is poor, apply the remedies in this order:
1. **Raise the antennas.** Height beats power (Day 28).
2. **Lower the air data rate.** More range, longer airtime — and then you must redo two calculations: the Day 44 timeout and the Day 29 duty-cycle budget. Changing one radio parameter always touches those two numbers.

**Check:** at least 95 % delivery at the real site, or a concrete fix plan.

### Day 70 🔧⚡ — Deployment
**Do.** Weatherproof box (cable glands, a desiccant bag against condensation, antenna vertical and as high as practical), battery in, valve connected to the water line. Run one full commanded cycle at the site.

**Check:** it watered, at the site, on command.

### Days 71–74 💻 — The observation week (30 minutes per day)
**Do.** Each day, read the data and compare with expectations:
- Delivery and retry trends.
- Battery voltage slope versus your Day 50 prediction. This comparison is the most honest exam of the whole project.
- Any restarts in the diagnostics, and their reset reasons.
- Adjust `moistureLowPct` to how your real soil behaves.

Known field problems, pre-diagnosed for you:
- Range drops after rain → wet leaves absorb the signal.
- Failures only around midday → temperature drift plus WiFi congestion at the base.
- Battery decline ending in restart storms → this is what the Day 67 low-battery rule prevents.

**Check:** seven clean days, or every anomaly explained.

---

## After Day 74

The next steps, ordered by value:
1. **OTA (over-the-air) firmware updates.** Reason it is first: every firmware fix currently requires a trip to the garden with a laptop. OTA removes that. It builds directly on the partition table you learned on Day 3.
2. **CI (continuous integration).** One configuration file makes a build server run `pio run` and `pio test -e native` on every change. Your project is already prepared for this.
3. **A second field node.** Two nodes can transmit at the same moment and collide. Your retry jitter (Day 42) already helps; now you can measure how far it carries. This is a genuinely interesting engineering problem.
4. **Port to ESP-IDF** (Espressif's professional framework, used for commercial products). Your protocol and logic libraries contain no hardware calls and are covered by tests, so they move almost unchanged. This was planned from Day 38.

The details of these steps are in the separate roadmap document.
