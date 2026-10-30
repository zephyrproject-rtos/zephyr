.. SPDX-FileCopyrightText: Copyright (c) 2026 Hsiu-Chi Tsai
.. SPDX-License-Identifier: Apache-2.0

ES8311 codec tests
##################

Run the register and fault-injection tests on both native widths::

   ./scripts/twister -T tests/drivers/audio/es8311 \
     -p native_sim/native -p native_sim/native/64 --inline-logs

The default scenario runs 86 cases. The ``repeat`` scenario shuffles the order,
repeats the suite three times and each case twice, for 516 case invocations per
configuration. Twister reports the 86 distinct case IDs in either scenario.

Five deferred-device initialization probes run once at application init. Their
return values, readiness, write counts and register snapshots are retained for
the tests to inspect, including during repetition. Start a new executable to
repeat the probes themselves; ``device_init()`` cannot initialize a device twice.

The emulator models registers, I2C failures and controlled transaction blocking.
It does not model analog settling, I2S data, clocks or power sequencing. These
tests do not replace board validation.
