# Porting MicroWorld

MicroWorld requires a C++17 compiler and a small standard-library subset. It
does not read a clock; the consumer supplies monotonic milliseconds at lifecycle
and update boundaries.

For a new toolchain or board:

1. compile public headers and a small consumer with strict warnings and without
   exceptions or RTTI;
2. confirm selected fixed capacities fit the application image and stack plan;
3. run unchanged behavior tests where the target supports them; and
4. record separately what was compiled and what was actually measured on
   hardware.

Do not replace readable branches, 64-bit time, or virtual hooks with clever
alternatives until a named measured budget justifies the change. Compile success
is never a claim about runtime timing, heap, stack, radio, or physical hardware.
