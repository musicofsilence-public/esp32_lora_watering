# Learning and Verification Log

Use this file for evidence produced while following
[`docs/esp32-lora-remote-controller-learning-guide.md`](docs/esp32-lora-remote-controller-learning-guide.md).

Do not record a guide value or assumption as if it was measured. Label each
statement as one of:

- **Prediction** — what you expect before the experiment
- **Calculation** — a result derived from stated inputs
- **Observation** — what you directly saw or measured
- **Decision** — an accepted choice and its evidence
- **Question** — something unresolved

## Confirmed hardware inventory

Add a row only after reading the marking or matching documentation.

| Item | Exact marking/model | Document and revision | Observed/confirmed on | Notes |
| --- | --- | --- | --- | --- |
| ESP32 module | | | | |
| Carrier/development board | | | | |
| Controller E32 | | | | |
| Valve-unit E32 | | | | |
| Antennas | | | | |
| Valve | | | | |
| Driver transistor/module | | | | |
| Power supplies | | | | |

## Accepted decision log

| Date | Decision | Evidence/rationale | Affected files/tests |
| --- | --- | --- | --- |

## Measurement summary

Keep units in every value and link each row to a detailed entry.

| Date | Stage | Measurement | Setup | Result | Entry |
| --- | --- | --- | --- | --- | --- |

## Failure-test summary

| Date | Failure injected | Expected safe result | Observed result | Pass? | Entry |
| --- | --- | --- | --- | --- | --- |

## Experiment entry template

Copy this section for each experiment.

### YYYY-MM-DD — Stage N: Short experiment name

**Purpose**

What question does this experiment answer?

**Environment and setup**

- Git commit/worktree state:
- PlatformIO Core:
- Espressif platform:
- ESP-IDF:
- Toolchain:
- Firmware role/build environment:
- Board/module:
- Wiring or simulator:
- Instruments and relevant accuracy:
- Radio configuration and legal basis, if applicable:

**Prediction**

State the expected result before running the experiment and explain why.

**Procedure**

List the exact build, test, wiring, or measurement steps. Note any deviation
from the learning guide.

**Observation**

Record raw output, timestamps, voltages, currents, counts, traces, or photos.
Separate direct observations from interpretation.

**Comparison and explanation**

Did the result match the prediction? Explain every meaningful difference.

**Safety check**

- What output/state was safety-critical?
- How was the safe condition verified?
- Was real valve power, water, or RF transmission involved?

**Result**

- Pass/fail:
- Acceptance criterion:
- Remaining uncertainty:

**Next question**

Write the one most useful next experiment or decision.
