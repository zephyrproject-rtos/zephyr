.. zephyr:board:: max32660evsys

Overview
********
The MAX32660 evaluation system offers a compact development platform that
provides access to all the features of the MAX32660 in a tiny, easy to
use board. A MAX32625PICO-based debug adapter comes attached to the main
board. It can be snapped free when programming is complete. The debug
module supports an optional 10-pin Arm® Cortex® debug connector for DAPLink
functionality. Combined measurements are 0.65in x 2.2in, while the main board
alone measures 0.65in x 0.95in. External connections terminate in a dual-row
header footprint compatible with both thru-hole and SMT applications. This
board provides a powerful processing subsystem in a very small space that
can be easily integrated into a variety of applications.

The Zephyr port is running on the MAX32660 MCU.

Hardware
********

- MAX32660 MCU:

  - High-Efficiency Microcontroller for Wearable Devices

     - Internal Oscillator Operates Up to 96MHz
     - 256KB Flash Memory
     - 96KB SRAM, Optionally Preserved in Lowest Power Backup Mode
     - 16KB Instruction Cache
     - Memory Protection Unit (MPU)
     - Low 1.1V VCORE Supply Voltage
     - 3.6V GPIO Operating Range
     - Internal LDO Provides Operation from Single Supply
     - Wide Operating Temperature: -40°C to +105°C

  - Power Management Maximizes Uptime for Battery Applications

     - 85µA/MHz Active Executing from Flash
     - 2µA Full Memory Retention Power in Backup Mode at VDD = 1.8V
     - 450nA Ultra-Low Power RTC at VDD=1.8V
     - Internal 80kHz Ring Oscillator

  - Optimal Peripheral Mix Provides Platform Scalability

     - Up to 14 General-Purpose I/O Pins
     - Up to Two SPI
     - I2S
     - Up to Two UARTs
     - Up to Two I2C, 3.4Mbps High Speed
     - Four-Channel Standard DMA Controller
     - Three 32-Bit Timers
     - Watchdog Timer
     - CMOS-Level 32.768kHz RTC Output

- Benefits and Features of MAX32660-EVSYS:

  - DIP Breakout Board

     - 100mil Pitch Dual Inline Pin Headers
     - Breadboard Compatible

  - Integrated Peripherals

     - Red Indicator LED
     - User Pushbutton

  - MAX32625PICO-Based Debug Adapter

     - CMSIS-DAP SWD Debugger
     - Virtual UART Console

Supported Features
==================

The ``max32660evsys`` board supports the following interfaces:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock and reset control             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial                              |
+-----------+------------+-------------------------------------+

Programming and Debugging
*************************

Flashing
========

An Arm® debug access port (DAP) provides an external interface for debugging during application
development. The DAP is a standard Arm CoreSight® serial wire debug port, uses a two-pin serial
interface (SWDCLK and SWDIO), and is accessed through 10-pin header (J4).

Once the debug probe is connected to your host computer, then you can simply run the
``west flash`` command to write a firmware image into flash.

.. note::

   This board uses OpenOCD as the default debug interface. You can also use
   a Segger J-Link with Segger's native tooling by overriding the runner,
   appending ``--runner jlink`` to your ``west`` command(s). The J-Link should
   be connected to the standard 2*5 pin debug connector (J3) using an
   appropriate adapter board and cable.

Debugging
=========

Please refer to the `Flashing`_ section and run the ``west debug`` command
instead of ``west flash``.

References
**********

- `MAX32660EVSYS web page`_

.. _MAX32660EVSYS web page:
   https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/max32660-evsys.html
