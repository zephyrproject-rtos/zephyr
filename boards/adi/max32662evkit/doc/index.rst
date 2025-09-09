.. zephyr:board:: max32662evkit

Overview
********
The MAX32662 evaluation kit (EV kit) provides a platform for evaluating
the capabilities of the MAX32662 microcontroller, which is a cost-effective,
ultra-low power, highly integrated 32-bit microcontroller designed
for battery-powered edge devices.

The Zephyr port is running on the MAX32662 MCU.

Hardware
********

- MAX32662 MCU:

  - High-Efficiency Microcontroller for Low-Power High-Reliability Devices

    - 256KB Flash
    - 80KB SRAM, Optionally Preserved in LowestPower BACKUP Mode
    - 16KB Unified Cache
    - Memory Protection Unit (MPU)
    - Dual- or Single-Supply Operation: 1.7V to 3.6V
    - Wide Operating Temperature: -40°C to +105°C

  - Flexible Clocking Schemes

    - Internal High-Speed 100MHz
    - Internal Low-Power 7.3728MHz
    - Ultra-Low-Power 80kHz
    - 16MHz–32MHz (External Crystal Required)
    - 32.768kHz (External Crystal Required)
    - External Clock Inputs for CPU and Low-PowerTimer

  - Power Management Maximizes Uptime for Battery Applications

    - 50μA/MHz at 0.9V up to 12MHz (CoreMark®) inACTIVE Mode
    - 44μA/MHz at 1.1V up to 100MHz (While(1)) inACTIVE Mode
    - 2.15μA Full Memory Retention Current in BACKUPMode at VDDIO = 1.8V
    - 2.4μA Full Memory Retention Current in BACKUPMode at VDDIO = 3.3V
    - 350nA Ultra-Low-Power RTC
    - Wakeup from Low-Power Timer

  - Optimal Peripheral Mix Provides Platform Scalability

    - Up to 21 General-Purpose I/O Pins
    - 4-Channel, 12-Bit, 1Msps ADC
    - Two SPI Controller/Target
    - One I2S Controller/Target
    - Two 4-Wire UART
    - Two I2C Controller/Target
    - One CAN 2.0B Controller
    - 4-Channel Standard DMA Controller
    - Three 32-Bit Timers
    - One 32-Bit Low-Power Timer
    - One Watchdog Timer
    - CMOS-Level 32.768kHz Calibration Output
    - AES-128/192/256 Hardware Accelerator

- Benefits and Features of MAX32662EVKIT:

  - 3-Pin Terminal Block for CAN Bus 2.0B
  - 128 x 128 (1.45in) Color TFT Display with SPI Interface
  - Selectable On-Board High-Precision Voltage Reference
  - USB 2.0 Micro-B to Serial UART
  - All GPIOs Signals Accessed through 0.1in Headers
  - Four Analog Inputs Accessed through 0.1in Header
  - SWD 10-Pin Header
  - Board Power Provided by USB Port
  - On-Board LDO Regulators
  - Individual Power Measurement on All IC Rails through Jumpers
  - One General-Purpose LED
  - One General-Purpose Pushbutton Switch

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| Name      | Name          | Settings      | Description                                                                                      |
+===========+===============+===============+==================================================================================================+
| JP1       | VREF EN       |               |                                                                                                  |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------------------+   |
|           |               | | 1-2       | |  | Connects the external voltage reference to the VREF pin; must be enabled in the software. |   |
|           |               | |           | |  | See the External Voltage Reference (VREF) section for additional information.             |   |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------------------+   |
|           |               | | Open      | |  | Disconnects the external voltage reference.                                               |   |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------------------+   |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP2       | I2C1_SCL_PU   | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 1-2       | |  | Connects the pull-up to I2C1A_SCL (P0.6); sourced by V_AUX.                   |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects the pull-up from I2C1A_SCL (P0.6); sourced by V_AUX.              |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP3       | N/A           | N/A           |  Does not exist.                                                                                 |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP4       | I2C1_SDA_PU   | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 1-2       | |  | Connects the pull-up to I2C1A_SDA (P0.9); sourced by V_AUX.                   |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects the pull-up from I2C1A_SDA (P0.9); sourced by V_AUX.              |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP5       | LED0 EN       | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 1-2       | |  | Enables LED0.                                                                 |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disables LED0.                                                                |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP6       | CTS0A EN      | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 1-2       | |  | Connects the USB-to-serial bridge to UART0A_CTS (P0.20).                      |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects the USB-to-serial bridge from UART0A_CTS (P0.20).                 |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP7       | RX0A EN       | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 1-2       | |  | Connects the USB-to-serial bridge to UART0A_RX (P0.11).                       |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects the USB-to-serial bridge from UART0A_RX (P0.11).                  |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP8       | TX0A EN       | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 1-2       | |  | Connects the USB-to-serial bridge to UART0A_TX (P0.10).                       |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects the USB-to-serial bridge from UART0A_TX (P0.10).                  |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP9       | RTS0A EN      | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 1-2       | |  | Connects the USB-to-serial bridge to UART0A_RTS (P0.19).                      |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects the USB-to-serial bridge from UART0A_RTS (P0.19).                 |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP10      | VCORE EN      | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 1-2       | |  | Connects 1V1 to VCORE.                                                        |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects 1V1 from VCORE.                                                   |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP11      | VDDIO/VDDASEL | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 2-1       | |  | Connects 1V8 to V_AUX, VDDIO EN (JP12), and VDDA EN (JP13) jumpers.           |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 2-3       | |  | Connects 3V3 to V_AUX, VDDIO EN (JP12), and VDDA EN (JP13) jumpers.           |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP12      | VDDIO EN      | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 1-2       | |  | Connects the JP11 selected voltage to VDDIO.                                  |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects the voltage from VDDIO.                                           |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

An Arm® debug access port (DAP) provides an external interface for debugging during application
development. The DAP is a standard Arm CoreSight® serial wire debug port, uses a two-pin serial
interface (SWDCLK and SWDIO), and is accessed through 10-pin header (J3). Logic levels are set
to V_AUX (1V8 or 3V3), which is determined by the shunt placement on JP11. In addition,
the UART1A port can also be accessed through J3.


Once the debug probe is connected to your host computer, then you can simply run the
``west flash`` command to write a firmware image into flash. To perform a full erase,
pass the ``--erase`` option when executing ``west flash``.

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

- `MAX32662EVKIT web page`_

.. _MAX32662EVKIT web page:
   https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/max32662evkit.html
