.. _nrf-s2ram-sample:

nRF5x S2RAM demo
################

NOTE
****

>> THIS IS A NON-FULLY FUNCTIONAL SAMPLE <<

Please note that the sample is only restoring the CPU context needed to
continue the execution of the sample but it is not restoring any other core
peripheral (NVIC, RTC, MPU, etc...), that must be taken care of separately by
SoC or platform code, or device, that should be restored by the PM subsystem.
This means that on resume we cannot use any device or core peripheral (timers,
UART, etc..) so we use a GPIO LED instead.


Overview
********

This sample can be used as template and to show the suspend-to-RAM (S2RAM)
capability of Zephyr.

On boot the sample is turning the LED1 on and going immediately into S2RAM (the
board is powered off). On suspend the CPU context is saved in RAM (that is
retained) and a special marker is set in RAM to signal that the CPU has the
context saved in RAM.

When the Button1 is pushed the board restarts from the reset routine and checks
for the special marker. If the special marker is present, the CPU context is
restored from RAM and the execution continues from the point where it left on
suspend, turning the LED3 on.

Requirements
************

This application uses nRF5340 DK board for the demo.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf/s2ram
   :board: nrf5340dk_nrf5340_cpuapp
   :goals: build flash
   :compact:

Running:

1. Open UART terminal.
2. Power Cycle Device.
3. LED1 will be on and the device will automatically turn itself off (S2RAM)
4. Press Button 1 to wake the device and restart the application as if it had
   been powered back on and the LED3 will turn on.
