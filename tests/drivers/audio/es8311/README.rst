.. SPDX-FileCopyrightText: Copyright (c) 2026 Hsiu-Chi Tsai
.. SPDX-License-Identifier: Apache-2.0

ES8311 codec tests
##################

Run the register and fault-injection tests on both native widths::

   ./scripts/twister -T tests/drivers/audio/es8311 \
     -p native_sim/native -p native_sim/native/64 --inline-logs

The ``repeat`` scenario shuffles test order and reuses the driver, emulator and
worker-thread storage across three suite runs and two executions of each case.
This checks fixture isolation and worker cleanup across calls within one boot.
Twister reports distinct case IDs; repetition does not add coverage of clocks
or analog behavior.

Five deferred-device initialization probes run once at application init. Their
return values, readiness, write counts and register snapshots are retained for
the tests to inspect, including during repetition. Start a new executable to
repeat the probes themselves; ``device_init()`` cannot initialize a device twice.

The emulator models registers, I2C failures and controlled transaction blocking.
It does not model analog settling, I2S data, clocks or power sequencing. These
tests do not replace board validation.
