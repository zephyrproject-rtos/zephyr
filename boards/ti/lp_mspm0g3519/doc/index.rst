.. zephyr:board:: lp_mspm0g3519

Overview
********

MSPM0Gx51x microcontrollers (MCUs) are part of the MSP highly integrated, ultra-low-power 32-bit MCU
family based on the enhanced Arm® Cortex®-M0+ 32-bit core platform operating at up to 80-MHz frequency.
These cost-optimized MCUs offer high-performance analog peripheral integration, support extended temperature
ranges from -40°C to 125°C, and operate with supply voltages ranging from 1.62 V to 3.6 V.

Hardware
********

The MSPM0Gx51x devices provide up to 512KB embedded flash program memory with built-in error correction
code (ECC) and up to 128KB SRAM with a hardware parity option. These MCUs also incorporate a
memory protection unit, 7-channel DMA, math accelerator, and a variety of peripherals including

* Analog.

  * Two 12-bit 4-Msps ADCs.

  * Configurable internal shared voltage reference.

  * One 12-bit 1-Msps DAC.

  * Three high speed comparators with built-in reference DACs.

  * Two zero-drift zero-crossover op-amps with programmable gain.

* Digital.

  * Two 16-bit advanced control timers.

  * Nine general-purpose timers.

    * Two 16-bit general-purpose timers for QEI interface.

    * Four 16-bit general-purpose timers with low-power operation in STANDBY mode.

    * One 32-bit high resolution general-purpose timer.

    * Two 16-bit timers with deadband support and up to 12 PWM Channels.

  * Two windowed-watchdog timers.

  * One RTC with alarm and calendar modes.

* Data Integrity and Encryption.

  * One AES HW accelerator capable of CTR, CBC, and ECB modes.

  * One Cyclic Redundancy Check (CRC) accelerator.

  * One True Random Number Generator (TRNG).

* Communication.

  * Seven UARTs, two with support for advanced modes such as LIN and Manchester.

  * Three I2C supporting SMBUS/PMBUS and speeds up to FM+ (1Mbits/s).

  * Three SPI, one with max speed 32Mbits/s.

  * Two CAN interface supporting CAN 2.0 A or B and CAN-FD.

Zephyr uses the ``lp_mspm0g3519`` board for building LP_MSPM0G3519

Features
********

- Onboard XDS110 debug probe
- EnergyTrace technology available for ultra-low-power debugging
- 3 buttons, 1 LED and 1 RGB LED for user interaction
- Temperature sensor circuit
- Light sensor circuit
- External OPA2365 (default buffer mode) for ADC (up to 4 Msps) evaluation
- Onboard 32.768-kHz and 40-MHz crystals
- RC filter for ADC input (unpopulated by default)

Details on the MSPM0G3519 LaunchPad can be found on the `TI LP_MSPM0G3519 Product Page`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Building and Flashing
*********************

Building
========

Follow the :ref:`getting_started` instructions for Zephyr application development.

For example, to build the blinky application for the MSPM0G3519 LaunchPad:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lp_mspm0g3519
   :goals: build

The resulting ``zephyr.bin`` binary in the build directory can be flashed onto
MSPM0G3519 LaunchPad using the steps mentioned below.

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
   :board: lp_mspm0g3519
   :goals: debug

References
**********

- `TI MSPM0 MCU Page`_
- `MSPM0G3519 TRM`_

.. _TI MSPM0 MCU Page:
   https://www.ti.com/microcontrollers-mcus-processors/arm-based-microcontrollers/arm-cortex-m0-mcus/overview.html

.. _MSPM0G3519 TRM:
   https://www.ti.com/lit/slau846

.. _TI LP_MSPM0G3519 Product Page:
   https://www.ti.com/tool/LP-MSPM0G3519
