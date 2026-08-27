.. zephyr:code-sample:: frdm-mcxe31b-system-off
   :name: System Off (MCXE31B standby)
   :relevant-api: sys_poweroff counter_interface

   Use standby (the only low-power mode) on FRDM-MCXE31B and wake via an RTC alarm.

Overview
********

This sample demonstrates the use of :c:func:`sys_poweroff` on the NXP
FRDM-MCXE31B board.

Unlike SoCs that expose resumable suspend states, MCXE31B's only low-power mode
is **standby**, which is *reset-on-wake*: the run domain is powered down and a
wake event triggers a functional reset. The application therefore models standby
as system power-off rather than a Zephyr PM suspend state.

The sample arms an RTC alarm as the wake source, powers the SoC off, and the
RTC alarm later wakes (reboots) the board. On the next boot the sample reports
that it woke from standby and retrieves a cycle count from standby-retained
noinit memory. The sample repeats the cycle
:kconfig:option:`CONFIG_APP_TEST_CYCLES` times, which defaults to 10.

Requirements
************

The FRDM-MCXE31B board with a serial console connected.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/frdm_mcxe31b/system_off
   :board: frdm_mcxe31b
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   Cold boot
   Powering off in 3 s...
   Powering off in 2 s...
   Powering off in 1 s...
   Wake-up alarm set for 5 seconds (cycle 1/10)
   Powering off

   *** Booting Zephyr OS ***
   Woke from standby
   Retained standby cycle count: 1/10
   Powering off in 3 s...
   Powering off in 2 s...
   Powering off in 1 s...
   Wake-up alarm set for 5 seconds (cycle 2/10)
   Powering off
   ...

   *** Booting Zephyr OS ***
   Woke from standby
   Retained standby cycle count: 10/10
   Completed 10 standby cycles

The board powers off, the RTC alarm wakes it after 5 seconds, and the boot
banner reappears (a reset, not a resume). The standby-retained noinit counter
lets the sample stop after the configured number of standby cycles.
