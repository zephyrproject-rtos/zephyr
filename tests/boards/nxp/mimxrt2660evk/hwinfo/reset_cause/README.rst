.. SPDX-FileCopyrightText: Copyright 2026 NXP
.. SPDX-License-Identifier: Apache-2.0

RT2660 EVK HWINFO reset cause test
#################################

This ztest suite checks HWINFO reset-cause APIs and software reboot on the
MIMXRT2660 EVK. Two configurations exercise warm and cold reboot separately:

* ``boards.mimxrt2660evk.hwinfo.reset_cause`` requests a warm reboot. The
  ``POWERCON_SOC_CTRL.GPR_COLD[3]`` marker must survive, RESETCON must report
  software reset without escalation, and HWINFO must report ``RESET_SOFTWARE``.
* ``boards.mimxrt2660evk.hwinfo.reset_cause.cold`` enables
  ``CONFIG_TEST_RESET_CAUSE_COLD`` and requests a cold reboot. The GPR marker
  must be zero, RESETCON must report both ``RESET_SW`` and ``WARM_ESCA``, and
  HWINFO must report ``RESET_SOFTWARE | RESET_HARDWARE``. Software reset
  escalation must also be cleared by the cold reset.

Each configuration clears sticky causes, verifies that they are zero, and
writes a nonzero GPR marker before requesting reboot. A cold reset clears that
marker, so the cold test identifies the rebooted phase using sticky RESETCON
software-reset and escalation flags. A retained pending marker after a
non-escalated reset leads to a failed assertion rather than another reboot.
The suite checks raw sticky sources during setup, before any test case clears
them, and captures the HWINFO causes for later assertions. A reset error without
the expected software escalation fails setup. The latest ``RSTSTAT`` may report
only a pin reset and is not used to validate the entire reset sequence.

The EVK can report an additional ``RESET_PIN`` during software reboot, even
after sticky causes were cleared. Both configurations allow that flag while
requiring their expected software/hardware flags and rejecting other causes.
Other cases check supported causes, repeated reads, and idempotent clearing.

Start each configuration after programming and a debugger reset, or after
power-on reset. The initial state must have ``GPR_COLD[3]`` clear and must not
already contain the same sticky software-cold-reset flags: that state cannot
be distinguished from completion of the test's cold reboot.
After an interrupted run, clear the marker with a debugger before restarting.
Warm reboot also requires ``ESCAEN.RESET_SW`` to be clear; suite setup fails
if software resets are already configured to escalate to cold resets.

The test owns ``GPR_COLD[3]`` and clears it during suite teardown. It does not
use ``GPR_COLD[0]``, which the SoC startup uses for TCM configuration. No RAM
retention, watchdog, reset pin, shell, or custom host script is required.

Build both configurations with Twister::

   west twister -p mimxrt2660_evk/mimxrt2663/cm85 \
     -T tests/boards/nxp/mimxrt2660evk/hwinfo/reset_cause --build-only

To execute them, use Twister device testing with a hardware map describing
the board's flash runner and serial connection::

   west twister -p mimxrt2660_evk/mimxrt2663/cm85 \
     -T tests/boards/nxp/mimxrt2660evk/hwinfo/reset_cause \
     --device-testing --hardware-map hardware-map.yaml

Power-on reset
**************

Power-on reset remains a manual check. With either configuration flashed,
capture serial output while removing and restoring board power. The first
``Initial reset cause:`` line must include the ``RESET_POR`` bit defined in
``include/zephyr/drivers/hwinfo.h``. The suite prints this value before
clearing causes and starting its software-reset sequence. Automating the
power-on check requires host-controlled board power.
