.. SPDX-FileCopyrightText: Copyright 2026 NXP
   SPDX-License-Identifier: Apache-2.0

.. zephyr:code-sample:: rw612_pm_flash_check
   :name: RW612 PM flash check
   :relevant-api: subsys_pm flash_interface

   Check external NOR flash health across RW612 low-power modes.

Overview
********

This sample enters each Zephyr low-power state supported by the FRDM-RW612
(PM1 ``RUNTIME_IDLE``, PM2 ``SUSPEND_TO_IDLE`` and PM3 ``STANDBY``) in turn.
Before and after every entry/exit it erases, writes and reads back a scratch
sector of the external NOR flash to determine whether the flash is operating
normally, and prints a per-mode PASS/FAIL plus a final summary.  The judgment
is purely functional: if ``flash_erase``/``flash_write``/``flash_read`` work
after a wake, the flash is considered healthy.

On PM3 wake the FlexSPI controller configuration is lost (the ROM only
restores a minimal FCB-based LUT).  The flash driver's power-domain
``TURN_ON`` restore re-initializes the controller so the flash API keeps
working.

The scratch area used by the test is a ``scratch`` fixed partition defined
in :file:`app.overlay` at 48 MiB into the 64 MiB NOR, far from the XIP image.

Requirements
************

* FRDM-RW612 board
* External NOR flash (W25Q512JV) and the FlexSPI PM restore in the flash
  driver (``PM_DEVICE_ACTION_TURN_ON`` re-probe)

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/frdm_rw612/pm_flash_check
   :board: frdm_rw612
   :goals: build flash

Open a serial terminal at 115200 baud and reset the board.  Expected output
ends with::

   === SUMMARY: PASS (0 failures) ===

Notes
*****

* The sample forces each power state with ``pm_state_force()`` so the exact
  state under test is deterministic.  The last forced state persists, so the
  sample busy-waits at the end instead of returning to the idle thread (which
  would re-enter PM3 forever and release the SWD debug port).
* ``pm_state_force()`` is one-shot: the forced state is consumed on the first
  idle entry, so after the forced PM3 cycle the kernel may briefly re-enter a
  shallow state using the default policy before the sample thread resumes.
  The PM notifier therefore only reports the state under test.
* ``CONFIG_IDLE_STACK_SIZE`` is raised to 2048 because the flash ``TURN_ON``
  restore re-runs the SFDP probe in the idle-thread context at PM3 wake;
  the probe itself needs roughly 270 bytes of stack.  The same requirement
  applies to any application that binds this driver to a power domain (see
  the ``FLASH_MCUX_FLEXSPI_NOR`` Kconfig help).
