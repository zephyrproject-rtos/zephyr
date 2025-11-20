.. zephyr:board:: lp_mspm0g5187

Overview
********

MSPM0G511x microcontrollers (MCUs) are part of the MSP highly integrated, ultra-low-power 32-bit MCU
family based on the enhanced Arm® Cortex®-M0+ 32-bit core platform, operating at up to 80-MHz frequency.
These MCUs offer a blend of cost optimization and design flexibility for applications requiring 32KB to 128KB
of flash memory in small packages (down to 4 mm x 4 mm) or high pin count packages (up to 64 pins).
These devices include USB 2.0-FS interface, digital audio interface, cybersecurity enablers, high performance
integrated analog, and provide excellent low power performance across the operating temperature range.

Hardware
********

The MSPM0G51xx devices provide up to 128KB embedded flash program memory with built-in error correction
code (ECC) and up to 32KB SRAM with a hardware parity option. These MCUs also incorporate a
memory protection unit, 12-channel DMA, and a variety of peripherals including

* Analog.

  * One 12-bit 4-Msps ADCs.

  * Configurable internal shared voltage reference.

  * One high speed comparator with built-in reference DAC.

* Digital.

  * Two 16-bit advanced control timers.

  * Four general-purpose timers.

    * Two 16-bit general-purpose timers.

    * One 16-bit general-purpose timer with low-power operation in STANDBY mode.

    * One 16-bit timer with deadband support and up to 8 PWM Channels.

  * One basic software timer including 4 indenpendent configurable 16-bit counters.

  * Two windowed-watchdog timers.

  * One RTC with alarm and calendar modes.

* Data Integrity and Encryption.

  * One AES HW accelerator capable of CTR and CBC modes.

  * One Cyclic Redundancy Check (CRC) accelerator.

* Communication.

  * Two configurable serial interfaces (UNICOMM) supporting UART or I2C.

  * One configurable serial interface supporting UART or SPI.

  * One dedicated SPI interface up to 32 Mbits/s.

  * One digital audio interface (I2S) supporting controller and target mode.

  * One USB2.0 interface with full-speed (12-Mbps) compliant device and host mode.

Zephyr uses the ``lp_mspm0g5187`` board for building LP_MSPM0G5187

Features
********

- Onboard XDS110 debug probe
- EnergyTrace technology available for ultra-low-power debugging
- 3 buttons, 1 LED and 1 RGB LED for user interaction
- One microSD slot
- One microphone
- One I2S based audio ADC

Details on the MSPM0G5187 LaunchPad can be found on the `TI LP_MSPM0G5187 Product Page`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Building and Flashing
*********************

Building
========

Follow the :ref:`getting_started` instructions for Zephyr application development.

For example, to build the blinky application for the MSPM0G5187 LaunchPad:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lp_mspm0g5187
   :goals: build

The resulting ``zephyr.bin`` binary in the build directory can be flashed onto
MSPM0G5187 LaunchPad using the steps mentioned below.

Flashing
========

OpenOCD can be used to program the flash memory on the devices.

.. code-block:: console

   $ west flash --openocd <path to cloned dir>/src/openocd --openocd-search <path to cloned dir>/tcl

Flashing can also be done using JLINK.

.. code-block:: console

   $ west flash --runner jlink

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lp_mspm0g5187
   :goals: debug

References
**********

- `TI MSPM0 MCU Page`_
- `MSPM0G5187 TRM`_

.. _TI MSPM0 MCU Page:
   https://www.ti.com/microcontrollers-mcus-processors/arm-based-microcontrollers/arm-cortex-m0-mcus/overview.html

.. _MSPM0G5187 TRM:
   https://www.ti.com/lit/slau846

.. _TI LP_MSPM0G5187 Product Page:
   https://www.ti.com/tool/LP-MSPM0G5187
