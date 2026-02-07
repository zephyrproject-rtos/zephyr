.. zephyr:board:: pic32cm_pl10_cnano

Overview
********

The PIC32CM PL10 Curiosity Nano evaluation kit (EV10P22A) is a hardware platform for evaluating the PIC32CM
PL10 family of microcontrollers. This board features the PIC32CM6408PL10048 microcontroller mounted on the board.

Hardware
********

- PIC32CM6408PL10048 MCU
- Arm Cortex-M0+ Core with Speeds up to 24 MHz Across a Supply Voltage Range of 1.8-5.5V
- 64 KB In-System Self-Programmable Flash with 8 KB SRAM
- Operating Voltage: 1.8-5.5V
- Available in 48-Pin TQFP and 48-Pin VQFN WF Packages
- Low Power
   - Idle and Standby sleep modes
   - Sleep Walking peripherals
- Multi-Voltage I/O (MVIO)
- Peripheral Touch Controller (PTC) with 25 Self-Capacitance Channels
- One 12-bit, 800 ksps Analog-to-Digital Converter (ADC)
- Two Analog Comparators (AC) with Window Compare Function
- Four 16-bit Timer/Counters:
   - Three 16-bit basic Timer/Counters (TC0-2)
   - One 16-bit Timer/Counter for Control with four PWM channels (TCC0)
- 32-bit Real Time Counter (RTC) with Clock/Calendar Function
- Two Serial Communication Interfaces (SERCOM), Configurable to Operate as:
   - USART with full-duplex and single-wire half-duplex configuration
   - ISO7816 UART
   - I2C up to 1 MHz (SERCOM0)
   - SPI
   - LIN Host/Client
   - RS-485
- Two-Channel Direct Memory Access Controller (DMAC) with CRC Generator
- Four-channel Event System (EVSYS)
- Configurable Custom Logic (CCL) with Four Look-Up Tables (LUT)
- Programming and Debugging Interface Disable (PDID) Functionality
- Watchdog Timer (WDT)
- External Crystal Oscillator with Failure Detection
- Integrated Temperature Sensor

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `PIC32CM PL10 Curiosity Nano User Guide`_ has detailed information about board connections.

Programming & Debugging
***********************

.. zephyr:board-supported-runners::

Setting Up the Debug Interface
==============================

PyOCD Setup
===========

1. Install Device Pack

   - Add support for the PIC32CM family devices using the following command:

   .. code-block:: console

      pyocd pack install pic32cm

2. Verify Device Support

   - Confirm that the target is recognized:

   .. code-block:: console

      pyocd list --targets

   - You should see an entry similar to:

   .. code-block:: text

      pic32cm6408pl10048        Microchip                PIC32CM6408PL10048           PIC32CM-PL   pack

3. Connect the Board

   - Connect the DEBUG USB port on the board to your host machine.
   - This connection **power up the board** and provides access to the **on-board Embedded Debugger (EDBG)**,
     which enables programming and debugging of the target microcontroller through PyOCD.

Building and Flashing the Application
=====================================

1. Build the Application

   You can build a sample Zephyr application, such as **Blinky**, using the ``west`` tool.
   Run the following commands from your Zephyr workspace:

   .. code-block:: console

      west build -b pic32cm_pl10_cnano -p -s samples/basic/blinky

   This will build the Blinky application for the ``pic32cm_pl10_cnano`` board.

2. Flash the Device

   Once the build completes, flash the firmware using:

   .. code-block:: console

      west flash

3. Observe the Result

   After flashing, **LED0** on the board should start **blinking**, indicating that the
   application is running successfully.

References
**********

PIC32CM PL10 Product Page:
    https://www.microchip.com/en-us/product/PIC32CM6408PL10048

PIC32CM PL10 Curiosity Nano evaluation kit Page:
    https://www.microchip.com/en-us/development-tool/ev10p22a

.. _PIC32CM PL10 Curiosity Nano User Guide:
    https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/UserGuides/PIC32CM-PL10-UserGuide-DS50004003.pdf
