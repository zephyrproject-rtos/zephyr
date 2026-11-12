.. zephyr:code-sample:: frdm-mcxe31b-system-off
   :name: System Off (MCXE standby)
   :relevant-api: sys_poweroff counter_interface gpio_interface

   Enter standby on FRDM-MCXE31B or FRDM-MCXE32B and wake via RTC or GPIO.

Overview
********

This sample demonstrates the use of :c:func:`sys_poweroff` on the NXP
FRDM-MCXE31B and FRDM-MCXE32B boards. MCXE32B power-off is owned by CPU0.

Unlike SoCs that expose resumable suspend states, these devices' only low-power mode
is **standby**, which is *reset-on-wake*: the run domain is powered down and a
wake event triggers a functional reset. The application therefore models standby
as system power-off rather than a Zephyr PM suspend state.

The sample arms an RTC alarm as the wake source, powers the SoC off, and the
RTC alarm later wakes (reboots) the board. On the next boot the sample reports
that it woke from standby and retrieves a cycle count from standby-retained
noinit memory. The sample repeats the cycle
:kconfig:option:`CONFIG_APP_TEST_CYCLES` times, which defaults to 10.

Power-cycle the board before starting a new validation run. A debugger or pin
reset during a cycle is not a standby wake and does not clear standby-retained
data.

Requirements
************

An FRDM-MCXE31B or FRDM-MCXE32B board with a serial console connected.

On FRDM-MCXE32B, GPIO wake-up uses **PTE0** (Arduino D12), mapped to WKPU
source 30. Connect a switch or a 3.3 V-compatible signal between this pin and
ground. The sample enables an internal pull-up and wakes on a falling edge.
Keep the input high until ``Wake-up GPIO PTE0 armed`` is printed, then
pull it low briefly and release it before the next cycle. The on-board SW3
button is PTD5, which is not a WKPU input and cannot wake the device from standby.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/frdm_mcxe31b/system_off
   :board: frdm_mcxe31b
   :goals: build flash
   :compact:

For FRDM-MCXE32B with CPU1 left stopped:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/frdm_mcxe31b/system_off
   :board: frdm_mcxe32b/mcxe32b/cpu0
   :goals: build flash
   :compact:

The default wake-up source is RTC. Set :kconfig:option:`CONFIG_APP_WAKEUP_GPIO`
to ``y`` for GPIO-only wake-up, or :kconfig:option:`CONFIG_APP_WAKEUP_BOTH`
for either source. GPIO-only runs require one external falling edge per cycle;
there is no RTC fallback that could hide a missing GPIO wake-up.

The RTC uses undivided SIRC, which keeps running in standby. A divided RTC
clock does not provide a working standby alarm.

Dual-core operation
===================

Use sysbuild to build both MCXE32B images:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/frdm_mcxe31b/system_off
   :board: frdm_mcxe32b/mcxe32b/cpu0
   :goals: build flash
   :west-args: --sysbuild
   :compact:

Sysbuild enables :kconfig:option:`CONFIG_APP_USE_CPU1` and flashes CPU1 before
CPU0. The sample reserves the first 2 MiB of flash for CPU0 and the remaining
1872 KiB for CPU1. Do not enable this option without flashing the companion
image.
The companion's vector table starts at 0x00600000; it does not reserve CPU0's
boot-header gap.

For this dual-core demonstration the private SRAM regions are naturally aligned
for the MPU: 32 KiB at 0x20408000 for CPU0 and 16 KiB at 0x2042c000 for CPU1.
The MPU keeps the shared standby SRAM non-cacheable while the cores' private
data caches remain enabled. The board's default SRAM layout is unchanged.

Before each standby entry, CPU0 starts CPU1, then requests shutdown using
a mailbox in non-cacheable standby SRAM. CPU1 disables interrupts, drains its
data cache, and executes WFI. CPU0 waits for both the acknowledgment and the
MC_ME WFI status before gating CPU1's clock. A handshake or clock transition
timeout is reported as a failure rather than forcing an active core off.

The SoC power-off hook refuses to enter standby while CPU1's clock is enabled.
Busy flash programming or erase is also rejected.
Applications other than this sample must provide their own inter-core shutdown
protocol and stop ongoing DMA and peripheral communication before calling
:c:func:`sys_poweroff`. An ordinary idle WFI is not a shutdown acknowledgment.

Standby does not retain kernel context, normal SRAM, or TCM. Both cores restart
from their images after wake-up; only the 32 KiB standby SRAM is retained. The
sample validates the retained counter and reports the WKPU wake-up flags
captured before driver initialization, along with the restored run clock.
UART results show functional entry and wake-up, not a standby-current measurement.

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
