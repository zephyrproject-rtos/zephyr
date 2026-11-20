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

Each address is checked with one zero-length write before it is used, so no
data is written to anything on the bus.  The check runs on the worker thread,
so a driver that never returns from it cannot stop the run.

The API does not require a driver to support zero-length writes, so the suite
treats them as a precondition.  If the absent address does not refuse one,
every test skips with that reason printed rather than failing.  If no
known-good address answers, only the tests that need one skip.

Which error codes pass
**********************

The I2C API documents ``-EIO`` for a failed transfer and ``-ETIMEDOUT`` for
one that reached the transfer timeout, so by default a failure must return
one of those two.  ``CONFIG_I2C_FAULT_TEST_RELAXED_ERRNO`` accepts any
negative value instead, for bringing up a driver that is not yet conformant.
Either way, each condition prints the value the driver returned.

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

Run it on any board that declares the ``i2c-dut`` alias:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/i2c/i2c_fault_injection
   :board: <board>
   :goals: build flash
   :compact:

Or through twister:

.. code-block:: console

   west twister -p <board> --device-testing --device-serial <port> \
           -T tests/drivers/i2c/i2c_fault_injection

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

Tests that need a known-good address skip when the line reads ``none found``.

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
