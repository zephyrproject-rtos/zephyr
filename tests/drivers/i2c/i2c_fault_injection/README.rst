.. SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
   SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
   SPDX-License-Identifier: Apache-2.0

.. _i2c_fault_injection:

I2C fault injection
###################

Overview
********

This suite drives an I2C controller into failure conditions and checks how
its driver responds.  It checks that the driver returns within the test wait
limit, ``CONFIG_I2C_FAULT_TEST_TIMEOUT_MS``, and leaves the bus and itself
usable afterwards.  Where the API documents the result, it also checks that
the failure is reported; elsewhere it records the value the driver returned.

The wait limit catches a driver that hangs instead of returning an error.
Without it, such a driver shows up only as a twister timeout, which looks the
same as a build or flash problem.

The suite runs on a single I2C controller.  It sends transfers to an address
that nothing answers, to the general call address (0x00), to the reserved
addresses (0x01 to 0x07 and 0x78 to 0x7F) and to a 10-bit address.  It also
calls the API with no messages, with an empty message between two others, with
an unassigned speed code and with a NULL buffer, and registers a target.  When
a board cannot run a test, the test skips and the log says why.

With two spare GPIOs wired to the bus, the suite also holds SDA, SCL or both
low.  It checks that a transfer on a held bus fails rather than hangs, and
that ``i2c_recover_bus()`` reports a bus it cannot clear and clears one it
can.  The tests that call ``i2c_recover_bus()`` on an idle bus need no wiring,
and skip on a driver that returns ``-ENOSYS``.

The 10-bit test skips on ``-ENOTSUP`` or ``-ENOSYS``, and when its address
answers the zero-length write sent before the test's data.  A controller that
rejects 10-bit messages with any other error passes it without sending a
10-bit address.

Devicetree contract
*******************

A board supplies an overlay under ``boards/`` that points the ``i2c-dut`` alias
at an enabled I2C controller.  The build fails if the alias names a disabled
node, and twister filters out boards without the alias.

.. code-block:: devicetree

   / {
           aliases {
                   i2c-dut = &i2c0;
           };
   };

For the tests that hold a line, the overlay also names two spare GPIOs under
the ``zephyr,user`` node, wired to the SDA and SCL nets of the controller
under test and declared ``GPIO_ACTIVE_HIGH``:

.. code-block:: devicetree

   / {
           zephyr,user {
                   scl-gpios = <&gpio0 2 GPIO_ACTIVE_HIGH>;
                   sda-gpios = <&gpio0 3 GPIO_ACTIVE_HIGH>;
           };
   };

Addresses
*********

The address-phase tests need an address that nothing on the bus answers.
The suite uses ``CONFIG_I2C_FAULT_TEST_ABSENT_ADDR``, by default 0x01, the
CBUS address.  The I2C-bus specification does not allow I2C-bus compatible
devices to respond to it, though a system may assign it to a target when CBUS
is not used (UM10204 Rev. 7.0, Table 4 and section 3.1.12).  The known-good
transfers need an address that answers.  The suite uses
``CONFIG_I2C_FAULT_TEST_PRESENT_ADDR`` where set, and otherwise the first
enabled devicetree child of the controller under test that answers.  No other
address is probed.

Each address is checked with one zero-length write before it is used, so
setup writes no data to anything on the bus.  The check runs on the worker
thread, so a driver that never returns from it cannot stop the run.

The tests that hold a line are the exception.  Each sends a write of
``CONFIG_I2C_FAULT_TEST_PAYLOAD_LEN`` bytes to the known-good address while
SDA, SCL or both are held low.  The data reaches that device only if the hold
does not take, for example when a forcing wire is off, and the test then
fails because the write succeeded.

The API does not require a driver to support zero-length writes, so the suite
treats them as a precondition.  If the absent address does not refuse one,
every test skips with that reason printed rather than failing.  If no
known-good address answers, only the tests that need one skip.

Which error codes pass
**********************

The I2C API documents ``-EIO`` for a failed transfer and ``-ETIMEDOUT`` for
one that reached the transfer timeout, so by default a failure must return
one of those two.  It documents ``-EBUSY`` for a bus that is not clear after a
recovery attempt, which the recovery test with SDA held requires.
``CONFIG_I2C_FAULT_TEST_RELAXED_ERRNO`` accepts any negative value instead,
for bringing up a driver that is not yet conformant.  Either way, each
condition prints the value the driver returned.

Line forcing
************

The ``bus_fault`` scenarios hold the bus lines through the GPIOs named under
``zephyr,user``.  Devicetree names the pins but cannot say that a jumper is
fitted, so each scenario declares the wiring with
``CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED`` and claims the ``i2c_bus_fault``
fixture.  Without it, the tests that hold a line skip.  Twister selects these
scenarios only on boards whose overlay names both GPIOs, and a build that sets
the option without them fails.  ``bus_fault.shield.cy15framkit_001`` also
needs the shield; see its section below.

A forcing GPIO is an output only while it holds a line low.  Setup asserts
that both lines read high, because an idle bus is pulled high and a low line
means the bus is already stuck or a wire is on the wrong net.  Each board
overlay that supports the fixture describes its wiring.

When a transfer hangs
*********************

Faulted transfers run on a worker thread, and the test thread waits for each
one for at most the wait limit.  A transfer still running at the wait limit
counts as a hang.

The API documents a bounded return only for a driver that selects
``I2C_TRANSFER_TIMEOUT_SUPPORTED`` with a non-zero timeout, taken from
``zephyr,transfer-timeout-ms`` or ``CONFIG_I2C_TRANSFER_TIMEOUT_MS``.  A hang
fails the condition only where that holds and the timeout is shorter than the
wait limit.  Elsewhere the condition skips.

Building and running
********************

Every scenario needs a board overlay under ``boards/`` that sets the
``i2c-dut`` alias.  To build the default configuration directly:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/i2c/i2c_fault_injection
   :board: <board>
   :goals: build flash
   :compact:

The scenarios below run through twister.  Each one is selected with ``-s``
together with ``-T``.  The tests that need a known-good address skip without
one (see Addresses).

basic
=====

Default configuration.  Needs no wiring.

.. code-block:: console

   west twister -p <board> --device-testing --device-serial <port> \
           -T tests/drivers/i2c/i2c_fault_injection \
           -s drivers.i2c.fault_injection.basic

Without userspace, ``test_misuse_null_buffer`` skips:

.. code-block:: none

   START - test_misuse_null_buffer
     without CONFIG_USERSPACE the API does not validate this argument and the driver may dereference it; enable CONFIG_I2C_FAULT_TEST_UNSAFE_API_MISUSE to run this test anyway
    SKIP - test_misuse_null_buffer in 0.016 seconds

userspace
=========

Builds with ``CONFIG_USERSPACE=y``, so ``test_misuse_null_buffer`` runs from a
user thread.  Needs no wiring, and runs only on an architecture with
userspace support (``CONFIG_ARCH_HAS_USERSPACE``).

.. code-block:: console

   west twister -p <board> --device-testing --device-serial <port> \
           -T tests/drivers/i2c/i2c_fault_injection \
           -s drivers.i2c.fault_injection.userspace

``test_misuse_null_buffer`` expects the syscall layer to reject the NULL
buffer and kill its thread, so the log carries a fatal error that the test
reports as expected.  The scenario sets ``ignore_faults`` so that twister
does not fail it for that.  From ``kit_pse84_eval/pse846gps2dbzc4a/m33``, with
the register dump left out:

.. code-block:: none

   START - test_misuse_null_buffer
   E: syscall copy_msgs_and_transfer failed check: Memory region 0 (size 8) read access denied
   ...
   E: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
   E: Current thread: 0x34038278 (test_misuse_null_buffer)
   Caught system error -- reason 3 1
   Fatal error expected as part of test case.
    PASS - test_misuse_null_buffer in 0.080 seconds

relaxed_errno
=============

Accepts any negative errno from a failed transfer or recovery (see Which
error codes pass).  Needs no wiring.

This scenario is for bringing up a driver that does not yet return the
documented error codes.  It shows where the driver hangs or breaks the bus
apart from which values it returns.  A pass here does not show that the
driver conforms; use the strict scenarios for that.

.. code-block:: console

   west twister -p <board> --device-testing --device-serial <port> \
           -T tests/drivers/i2c/i2c_fault_injection \
           -s drivers.i2c.fault_injection.relaxed_errno

The output matches ``basic``.  Each test still prints the value the driver
returned, so a non-conformant value is visible even when the test passes.

bus_fault
=========

Adds the tests that hold a line low and recover the bus.  The hardware it
needs:

- Two spare GPIOs, named by ``sda-gpios`` and ``scl-gpios`` under
  ``zephyr,user`` in the board overlay, wired to the SDA and SCL nets of the
  controller under test.  The board overlay describes where.
- A device on the bus that answers, for the tests that write while a line is
  held.
- A driver that implements ``i2c_recover_bus()``, for the recovery tests.
  Some drivers need recovery GPIOs in devicetree for this.

The ``i2c_bus_fault`` fixture tells twister the wiring is in place.

.. code-block:: console

   west twister -p <board> --device-testing --device-serial <port> \
           --fixture i2c_bus_fault \
           -T tests/drivers/i2c/i2c_fault_injection \
           -s drivers.i2c.fault_injection.bus_fault

The environment reports line forcing as available.  From
``kit_pse84_eval/pse846gps2dbzc4a/m33``, two of the recovery tests:

.. code-block:: none

   I2C fault injection environment
     controller under test: scb@52990000
     absent address:        0x01
     known-good address:    0x68
     line forcing:          available
   ...
   START - test_recovery_both_held
   W: I2C abort did not confirm controller idle; bus may be stuck
     both lines held low: driver returned -116
    PASS - test_recovery_both_held in 0.521 seconds
   ===================================================================
   START - test_recovery_fails_while_sda_held
   E: failed to recover bus (err -16)
     recovery with SDA held: driver returned -16
    PASS - test_recovery_fails_while_sda_held in 0.009 seconds

Lines that start with ``W:`` or ``E:`` are logged by the driver under test
and differ between drivers.

shield.cy15framkit_001
======================

Builds in the CY15FRAMKIT-001 shield, whose FM24W256 at 0x51 is then a
known-good device.  Fit the shield on the board's Arduino header.  The
scenario runs on boards that support the Arduino I2C header and whose
overlay points ``i2c-dut`` at that controller.  If the controller has another
device in devicetree that answers first, that device is the known-good one
instead.  The ``cy15framkit_001`` fixture tells twister the shield is fitted.

.. code-block:: console

   west twister -p <board> --device-testing --device-serial <port> \
           --fixture cy15framkit_001 \
           -T tests/drivers/i2c/i2c_fault_injection \
           -s drivers.i2c.fault_injection.shield.cy15framkit_001

The environment shows the shield's FM24W256 as the known-good device:

.. code-block:: none

   I2C fault injection environment
     controller under test: scb@52820000
     absent address:        0x01
     known-good address:    0x51

bus_fault.shield.cy15framkit_001
================================

``bus_fault`` with the CY15FRAMKIT-001 shield built in and fitted, for boards
whose controller under test has no device of its own to answer while a line
is held.  Needs both the forcing wiring and the shield, so it claims both
fixtures.

.. code-block:: console

   west twister -p <board> --device-testing --device-serial <port> \
           --fixture i2c_bus_fault --fixture cy15framkit_001 \
           -T tests/drivers/i2c/i2c_fault_injection \
           -s drivers.i2c.fault_injection.bus_fault.shield.cy15framkit_001

Console output
**************

Every result depends on the controller and the two addresses the suite
settles on, so it prints them before the first test.  Check them first.
From a run on ``kit_pse84_eval/pse846gps2dbzc4a/m33``, where the known-good
address is the board's BMI270:

.. code-block:: none

   I2C fault injection environment
     controller under test: scb@52990000
     absent address:        0x01
     known-good address:    0x68
     line forcing:          not wired

Tests that need a known-good address skip when the line reads ``none found``,
and tests that hold a line skip when line forcing reads ``not wired``.

Each test prints the value the driver returned:

.. code-block:: none

     address NACK on write: driver returned -5

A faulted transfer that does not return within the wait limit is reported
with the condition's name and "transfer did not return within <N> ms; the
driver hung rather than reporting an error".  The suite then waits one more
wait limit for the transfer to return.  If it does, the run continues.  If
not, the remaining conditions skip.  Faulted transfers run on a single worker
thread, which cannot start another while the stuck call has not returned, and
the state of the bus is unknown.
