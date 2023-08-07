.. _mimxrt595_evk-system-off-sample:

RT595 System Off demo
#####################

Overview
********

This sample can be used for basic power measurement and as an example of
soft off on NXP i.MX RT platforms. The functional behavior is:

* Busy-wait for 2 seconds
* Turn the system off after enabling wakeup through RTC and set alarm
  10 seconds in the future to wake up the processor

Requirements
************

This application uses MIMXRT595-EVK for the demo.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/mimxrt595_evk_cm33/system_off
   :board: mimxrt595_evk_cm33
   :goals: build flash
   :compact:

Running:

1. Open UART terminal.
2. Power Cycle Device.
3. Device will turn on and idle for 2 seconds
4. Device will turn itself off using deep power down mode. RTC alarm
   will wake device and restart the application from a warm reset.

Sample Output
=================
MIMXRT595-EVK core output
--------------------------

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.2.0-1045-g07228f716c78  ***

   Wake-up alarm set for 10 seconds
   Powering off

OTP Fuse setting to wake from Deep Power Down and reset flash
#############################################################

Background
**********

This sample will not resume the application after waking from Deep Power
Down (DPD) mode on an EVK with default settings. The reason is that the
flash is not normally reset when waking from DPD. This sample
eXecutes-In-Place (XIP) from the external flash. When the MCU wakes from
DPD, it wakes through the reset flow. But if the external flash is not
reset, the MCU and flash are no longer in sync, and the MCU cannot XIP.
In this default state, one can confirm the RTC is waking the MCU from
DPD because the MCU will set the PMIC_MODE pins to 0b00 requesting the
PMIC to enter the default boot mode, and the PMIC will enable the
regulator driving the VDDCORE rail at 1.0V. However, the MCU will not be
able to XIP from the flash to resume. One can press the Reset button in
this state to restart the app.

To wake from DPD and resume XIP from the flash, the MCU needs to be
configured to reset the external flash. This can be done by programming
the One-Time-Programmable (OTP) fuses in the MCU. The steps below detail
how program the OTP BOOT_CFG1 fuses to use GPIO pin PIO4_5 as the flash
reset. This fuse setting instructs the ROM bootloader to toggle PIO4_5
when resetting the flash. Note that the MIMXRT595-EVK board is designed
to have PIO4_5 drive the octal flash reset pin on FlexSPI0. Other boards
using this MCU may use a different GPIO pin, and the setting in the OTP
fuses must match the GPIO pin connected to reset. Before programming
fuses, it is best to write the OTP shadow registers first and confirm
the operation. Then program the OTP fuses after confirming the correct
settings. For more details on OTP fuses and shadow registers, refer to
the Reference Manual for this MCU, and the OTP Fuse Map spreadsheet
included as an attachment in the Reference Manual PDF.

Tools needed
************
These steps use the blhost tool that runs on a host computer running
Linux, Windows, or MacOS. Download blhost, find the appropriate blhost
executable for your host OS, and use the command-line steps below to
program the OTP fuses. To download, go to https://www.nxp.com/mcuboot,
and find the Blhost package under Downloads.

Steps to program OTP fuses on MIMXRT595-EVK
*******************************************
These steps detail using USB as the interface between blhost and the
ROM bootloader. UART is another option, for more details see the
blhost documentation and the Boot ROM chapter in the MCU Reference
Manual.

 1. Power the EVK and connect USB J38 to host computer. J38 is for the
    USB peripheral of the MCU, and will also power the EVK.

 2. Set the DIP switches of SW7 to On-Off-On (123) to boot in ISP USB
    HID mode.

 3. Press the Reset button SW3 to boot in ISP mode. The EVK should
    enumerate as a USB HID device in the host computer.

 4. This command confirms the current settings of BOOT_CFG1 fuses:
    blhost -u 0x1fc9,0x0023 -- efuse-read-once 0x61

 5. This command programs BOOT_CFG1 to enable the flash reset pin using
    PIO4_5:
    blhost -u 0x1fc9,0x0023 -- efuse-program-once 0x61 0x164000

 6. This command confirms the programmed fuses in BOOT_CFG1:
    blhost -u 0x1fc9,0x0023 -- efuse-read-once 0x61

 7. Set the DIP switches of SW7 to Off-Off-On (123) to boot from the
    external flash on FlexSPI0.

 8. Press the Reset button SW3 to boot from flash and run the app.

Expected results from blhost
****************************

>blhost -u 0x1fc9,0x0023 -- efuse-read-once 0x61
Inject command 'efuse-read-once'
Response status = 0 (0x0) Success.
Response word 1 = 4 (0x4)
Response word 2 = 0 (0x0)

>blhost -u 0x1fc9,0x0023 -- efuse-program-once 0x61 0x164000
Inject command 'efuse-program-once'
Successful generic response to command 'efuse-program-once'
Response status = 0 (0x0) Success.

>blhost -u 0x1fc9,0x0023 -- efuse-read-once 0x61
Inject command 'efuse-read-once'
Response status = 0 (0x0) Success.
Response word 1 = 4 (0x4)
Response word 2 = 1458176 (0x164000)
