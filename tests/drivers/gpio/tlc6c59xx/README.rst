.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

TLC6C59XX GPIO simulator tests
##############################

These tests run the production GPIO and MFD drivers on ``native_sim/native/64``.
A test-local SPI emulator captures complete transfers, injects errors, and delays
transfers to allow concurrent writers to contend for the driver locks. No board
or programmer is required.

Run from the Zephyr repository in a west workspace::

   export ZEPHYR_BASE="$PWD"
   west twister -p native_sim/native/64 -T tests/drivers/gpio/tlc6c59xx -i

All scenarios use three GPIO children declared in physical chain order. The base
scenarios use 8-bit and 12-bit chips; the 12-bit chain exercises 36 outputs with
four leading padding bits. Additional 12-bit scenarios use active-low and
active-high reset GPIOs. MFD support is selected by the GPIO driver rather than
explicitly enabled in the test configuration.

Coverage includes:

* An all-zero SPI frame during initialization.
* Reset GPIO configuration and inactive/active/inactive levels before the first
  SPI transfer, using both reset polarities.
* Chain ordering, byte packing, and leading padding.
* Setting and clearing every output, including outputs beyond the first 32 bits.
* Masked writes, set, clear, toggle, output initialization, and active-low logic.
* SPI error propagation and rollback of both GPIO and parent cached state for
  each write operation, verified through subsequent successful transfers.
* Unsupported configurations, invalid positions, values and masks, and ISR calls.
* Concurrent writers on the same chip and on different chips, with a blocking
  SPI transfer to exercise both child and parent serialization.

The emulator does not model electrical output polarity, reset-pulse timing,
SPI clock edges, latch timing, or partially completed failed transfers. Invalid
devicetree layouts and initialization failures are not covered by these runtime
tests. Hardware testing is still required for electrical behavior.
