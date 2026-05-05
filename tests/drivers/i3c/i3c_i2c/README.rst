.. SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
..                         or an affiliate of Infineon Technologies AG.
.. SPDX-License-Identifier: Apache-2.0

.. _i3c_i2c_test:

I3C Controller: Legacy I2C Device Test Suite
############################################

Overview
********

Validates that an I3C controller correctly initialises legacy I2C devices
on the I3C bus and exposes them through the standard Zephyr I2C API.
The suite is hardware-driven: a real I2C target on the I3C bus loops
back to the controller under test.  This suite expects that the I3C instance
and both I2C instances are on the same board.

Hardware Setup (Board-Agnostic)
*******************************

The board overlay must provide:

``test-i3c``
  The I3C controller under test (controller role).

``test-i2c-target``
  An I2C target role on the same bus as ``test-i3c``.

``test-i2c-target2``
  (Optional) A second I2C target on the same bus, used by the
  multiple-targets test.

Wiring requirement: SDA/SCL of ``test-i3c``, ``test-i2c-target``, and (when
present) ``test-i2c-target2`` must be physically tied together with adequate
pull-ups (external resistors or pad-internal pull-ups).

The suite registers each ``i2c_target_config`` from the test code itself, so
the I2C target hardware does not need to be pre-configured by the board
overlay beyond enabling the peripheral and assigning pinctrl.

Coverage vs. I3C v1.x Specification (Legacy-I2C Subset)
********************************************************

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Spec Requirement
     - Covered By
   * - Controller drives legacy I2C device at static address using SDR
       open-drain timing
     - ``test_write_data_integrity``, ``test_read_data_integrity``
   * - Repeated START (Sr) between write and read
     - ``test_write_then_read``
   * - Multi-byte burst read with byte-level ACK/NACK
     - ``test_burst_sequential_read``
   * - Multiple legacy targets coexist on one bus
     - ``test_multiple_targets``
   * - Address NACK detection and reporting
     - ``test_address_probe`` (case c), ``test_bus_recovery``
   * - Controller halt + RESUME after a NACK
     - ``test_bus_recovery``
   * - Address-slot bookkeeping (no duplicates)
     - ``test_attach_detach`` (step 5)
   * - Subsystem gating before bus access for unknown addresses
     - ``test_address_probe`` (case b)
   * - Dynamic attach/detach of an LVR-described device
     - ``test_attach_detach``

Known Gaps and Not-Yet-Covered Areas
*************************************

The below list is only concerned with I2C-legacy related features — e.g. Hot
Join does not apply and is not listed.

* **I2C Fast-Mode-Plus (FM+) timing.**  The suite runs the controller's I2C
  output at the rate the overlay configures (typically 100 kHz / 400 kHz);
  it does not parameterise across SCL rates.

* **In-flight target unregistration.**  Calling ``i2c_target_unregister``
  while a transfer is blocked in ``k_sem_take`` is not exercised; instead
  ``test_bus_recovery`` approximates this by unregistering before the
  transfer.

* **Bus-arbitration loss.**  Requires a second active I3C controller, which
  is not part of the test fixture.

* **Power-state transitions (suspend/resume) mid-transfer.**
