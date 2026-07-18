# Porting MicroWorld

MicroWorld requires a conforming C++17 compiler and a small standard-library
subset: fixed arrays, fixed-width integers, size types, limits, and type traits.
The library does not read a clock. A consumer supplies a monotonic
`TimePointMilliseconds` to every lifecycle/update boundary.

For a new MCU or toolchain:

1. Compile every public header and source with high warnings.
2. Compile a consumer with exceptions and RTTI disabled.
3. Confirm fixed capacities fit static RAM and task stack budgets.
4. Measure disabled, all-due, mixed-rate, and maximum-capacity workloads after
   1,000 warm-up updates, for 10,000 measured updates across 30 trials.
5. Record compiler/platform versions, flags, optimization/LTO state, CPU
   frequency, object sizes, flash, static RAM, stack high-water mark, allocation
   delta, cycle-counter overhead, median, p95, and worst dispatch cost.
6. Run all behavior tests unchanged before retaining an optimization.

Target map and runtime output are target evidence; host timing cannot substitute
for them. Uploading or executing firmware requires the target owner's explicit
authorization. A successful compile alone must not be reported as measured
cycles, heap behavior, or stack margin.

Keep virtual dispatch, 64-bit time, ordinary booleans, and readable branches
unless a named target budget fails. Do not bit-pack state, narrow time, replace
all hooks with templates, or add allocators without before/after evidence that
justifies the added complexity.
