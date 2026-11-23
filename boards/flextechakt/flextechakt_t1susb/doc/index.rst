.. zephyr:board:: flextechakt_t1susb

Overview
********

The FlextechAKT 10BASE-T1S to USB Development Board is a platform to help engineers
quickly implement 10BASE-T1S and USB solutions using the MAX32690 Arm®
Cortex®-M4F and AD3306 10BASE-T1S MACPHY. The small form factor board features a
PMOD, Qwiic, and general purpose expansion headers.

The Zephyr port is running on the MAX32690 MCU.

.. note::

  The AD3306 10BASE-T1S MACPHY is hardware and software compatible with the
  ADIN1140 10BASE-T1S MACPHY.

Hardware
********

- MAX32690 MCU:

    - Ultra-Efficient Microcontroller for Battery-Powered Applications

      - 120MHz Arm Cortex-M4 Processor with FPU
      - 7.3728MHz and 60MHz Low-Power Oscillators
      - External Crystal Support (32MHz required for BLE)
      - 32.768kHz RTC Clock (Requires External Crystal)
      - 8kHz Always-On Ultra-Low Power Oscillator
      - 3MB Internal Flash, 1MB Internal SRAM (832kB ECC ON)
      - 85 μW/MHz ACTIVE mode at 1.1V
      - 1.8V and 3.3V I/O with No Level Translators
      - External Flash & SRAM Expansion Interfaces

    - Multiple Peripherals for System Control

      - 16-Channel DMA
      - Up To Five Quad SPI Master (60MHz)/Slave (48MHz)
      - Up To Four 1Mbaud UARTs with Flow Control
      - Up To Two 1MHz I2C Master/Slave
      - I2S Master/Slave
      - Eight External Channel, 12-bit 1MSPS SAR ADC w/ on-die temperature sensor
      - USB 2.0 Hi-Speed Device
      - 16 Pulse Train Engines
      - Up To Six 32-Bit Timers with 8mA High Drive
      - Up To Two CAN 2.0 Controllers
      - Up To Four Micro-Power Comparators
      - 1-Wire Master

    - Security and Integrity

      - ChipDNA Physically Un-clonable Function (PUF)
      - Modular Arithmetic Accelerator (MAA), True Random Number Generator (TRNG)
      - Secure Nonvolatile Key Storage, SHA-256, AES-128/192/256
      - Secure Boot ROM

- External devices connected on the FlextechAKT T1SUSB:

  - On-Board 24LC256 EEPROM
  - USB 2.0 Type-C interface to the MAX32690
  - I2C PMOD connector
  - I2C Qwiic connector
  - SWD 10-Pin Header
  - AD3306 (ADIN1140 compatible) Robust, Industrial, Low Power 10BASE-T1S MAC-PHY
  - On-Board 5V, 3.3V, 1.8V, and 1.1V voltage regulators
  - Board Power Provided by either the USB Port
  - Four general-purpose LEDs, one RGB LED and two general purpose push buttons

.. note::

  The MAX32690 MCU features an integrated Bluetooth LE Radio. This board does not
  feature an antenna or supporting components to enable BLE functionality.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

The MAX32690 MCU can be flashed by connecting an external debug probe to the
SWD port. SWD debug can be accessed through the Cortex 10-pin connector, P5.
Logic levels are fixed to VDDIO (1.8V).

Once the debug probe is connected to your host computer, then you can run the
``west flash`` command to write a firmware image into flash. Here is an example
for the :zephyr:code-sample:`hello_world` application. To perform a full erase,
pass the ``--erase`` option when executing ``west flash``.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: flextechakt_t1susb/max32690/m4
   :goals: flash

.. note::

   This board uses OpenOCD as the default debug interface. You can also use a
   Segger J-Link with Segger's native tooling by overriding the runner,
   appending ``--runner jlink`` to your ``west`` command(s). The J-Link should
   be connected to the standard 2*5 pin debug connector (P5) using an
   appropriate adapter board and cable.

Debugging
=========

Once the debug probe is connected to your host computer, then you can run the
``west debug`` command to write a firmware image into flash and start a debug
session. Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: flextechakt_t1susb/max32690/m4
   :goals: debug

References
**********

- `FlextechAKT T1SUSB Product Page`_
- `MAX32690 solution center`_

.. _FlextechAKT T1SUSB Product Page:
   https://flextechakt.com/products/adi-usb-to-e2b-10baset1s-development-board
.. _MAX32690 solution center:
   https://developer.analog.com/solutions/max32690
