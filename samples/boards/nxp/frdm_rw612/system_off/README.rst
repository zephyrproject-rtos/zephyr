.. SPDX-FileCopyrightText: Copyright 2026 NXP
   SPDX-License-Identifier: Apache-2.0

.. zephyr:code-sample:: rw612_system_off
   :name: FRDM-RW612 PM4 system off
   :relevant-api: sys_poweroff counter_interface

   Enter PM4 and verify a fresh boot after RTC wakeup.

Overview
********

This sample enters hardware PM4 through ``sys_poweroff()``. PM4 powers down
main SRAM and loses the Zephyr execution context. An RTC alarm wakes the
SoC through reset: the image starts again and reports
``RESET_LOW_POWER_WAKE`` through the hwinfo API. The call to
``sys_poweroff()`` never resumes.

On RW6xx, ``sys_poweroff()`` powers off both WLAN and BLE before entering
PM4. Wi-Fi and Bluetooth LE connections are terminated. Applications must
reinitialize the wireless subsystems and reestablish any required connections
after the wakeup reset.

Build and run
*************

Build for FRDM-RW612::

   west build -b frdm_rw612 samples/boards/nxp/frdm_rw612/system_off \
     -d build/rw612_system_off

Open the console at 115200 baud. The application waits ten seconds.
It then arms the RTC alarm for thirty seconds and enters PM4.
``CONFIG_SAMPLE_RW612_SYSTEM_OFF_WAKEUP_SECONDS`` changes this interval.
After wakeup the image prints::

   RW612 boot: reset cause 0x00000080
   PM4 wake reset: fresh boot; SYSTEM OFF TEST PASSED

The image then returns from ``main()`` without requesting another system off.
Reset the board to repeat the sequence. The hwinfo API checks the wake mode
together with the hardware reset cause. A pin or software reset after an
earlier PM4 cycle does not count as a new PM4 wakeup.

The application prepares its wake source before calling ``sys_poweroff()``.
It programs the RTC through the counter API and enables its PMU wakeup route
through the WUC API, using the RTC node's ``wakeup-ctrls`` property. This works
with both ``CONFIG_PM=n`` and ``CONFIG_PM=y``.
The RTC's ``wakeup-ctrls = <&wuc 32>`` uses ``RTC_IRQn``, whose value
is 32. WUC consumes peripheral IRQn values, without the Cortex-M vector
offset of 16. The driver routes the request through the PMU; the counter
API configures the alarm separately.
The platform hook leaves PMU wake sources armed, disables NVIC interrupts,
turns off WLAN/BLE and enters PM4 with the upper AON SRAM powered down.
Applications using other wake sources must configure those sources first.
FRDM-RW612 SW2 is GPIO11 and cannot wake PM4; external AON GPIO24/GPIO25
wiring is required for an AON pin wake test.

Current measurement
*******************

Measure current at JP9 after disconnecting the debugger. Close J-Link after
programming, then toggle only its reset pin without attaching the debug
port, using Commander ``-AutoConnect 0`` and ``r0``, ``sleep 100``, ``r1``,
``qc``. Discard startup, entry, wakeup and the completed-test current.

The overlay disables unused board peripherals. PM4 powers down the non-AON
GPIO pads, including GPIO0, GPIO1 and GPIO12 used by the RGB LEDs. No
application LED configuration is needed before entering PM4. See Table 2
in AN14464 for the GPIO pad behavior in each power mode.

The platform power-off hook powers down CAU in sleep when CLK32K uses RC32K
or XTAL32K. The default board configuration uses RC32K.
The RTC seconds alarm is a wake source; the high-resolution counter is
not enabled. PM4 powers down UART with the main peripheral domain; no explicit
UART suspend is required. The sample prints its entry message before arming
the RTC alarm and calls ``sys_poweroff()`` without an additional delay. The
last console bytes may be truncated when UART powers down.

The reset-to-success console sequence verifies functionality. It does not
check current limits. Retain a separate instrument capture for the PM4
interval. A voltage or power result also requires connected voltage-sense
leads; a current-only JP9 setup does not provide those quantities.

See `NXP AN14464: Low Power Checklist for RW61x Family
<https://www.nxp.com/docs/en/application-note/AN14464.pdf>`_ for PM4 reset
wakeup and board measurement boundaries.
