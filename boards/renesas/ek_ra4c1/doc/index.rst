.. zephyr:board:: ek_ra4c1

Overview
********

The EK-RA4C1, an Evaluation Kit for the RA4 Series, enables users to seamlessly evaluate
the features of the RA4C1 MCU group. The users can use rich on-board features along with
their choice of popular ecosystems add-ons to bring their big ideas to life.

The key features of the EK-RA4C1 board are categorized in three groups (consistent with
the architecture of the kit) as follows:

**Renesas RA4C1 Microcontroller Group**

- R7FA4C1BD3CFP MCU (referred to as RA MCU)
- 80 MHz, Arm® Cortex®-M33 core
- 512 KB Code Flash, 96 KB SRAM
- 100 pins, LQFP package
- Native pin access through 3 x 26-pin headers (not populated)
- Tamper Detection embedded into J4
- Segment LCD Board Interface
- MCU current measurement points for precision current consumption measurement
- Multiple clock sources – RA MCU oscillator and sub-clock oscillator crystals,
  providing precision 8.000 MHz and 32,768 Hz reference clocks. Additional low-precision
  clocks are available internal to the RA MCU

**System Control and Ecosystem Access**

- Two 5 V input sources

   - USB (Debug)
   - External Power Supply 2-pin header (not populated)

- Three Debug modes

   - Debug on-board (SWD)
   - Debug in (SWD)
   - Debug out (SWD, SWO and JTAG)

- User LEDs and buttons

   - Three User LEDs (red, blue, green)
   - Power LED (white) indicating availability of regulated power
   - Debug LED (yellow) indicating the debug connection
   - Two User buttons
   - One Reset button

- Five most popular ecosystems expansions

   - Two Seeed Grove® system (I2C/Analog) connectors (not populated)
   - SparkFun® Qwiic® connector (not populated)
   - Two Digilent PmodTM (SPI, UART and I2C) connectors
   - Arduino™ (UNO R3) connector
   - MikroElektronikaTM mikroBUS connector (not populated)

- MCU boot configuration jumper
- Low Voltage Mode voltage input and operation

**Special Feature Access**
- 32 MB (256 Mb) External Quad-SPI Flash
- CAN-FD (3-pin header)
- External Battery Connector
- Configuration Switch

Hardware
********

Detailed hardware features can be found at:
- RA4C1 MCU:  `RA4C1 Group User's Manual Hardware`_
- EK-RA4C1 board: `EK-RA4C1 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Program can be flashed to EK-RA4C1 via the on-board SEGGER J-Link debugger.
SEGGER J-link's drivers are available at https://www.segger.com/downloads/jlink/

To flash the program to board

   1. Connect to J-Link OB via USB port to host PC

   2. Make sure J-Link OB jumper is in default configuration as describe in `EK-RA4C1 - User's Manual`_.
      Note: SW4-4 needs to be set to OFF

   3. Execute west command

     .. code-block:: console

        west flash -r jlink

References
**********

.. target-notes::

.. _EK-RA4C1 Website:
   https://www.renesas.com/en/design-resources/boards-kits/ek-ra4c1

.. _RA4C1 MCU group Website:
   https://www.renesas.com/en/products/ra4c1

.. _EK-RA4C1 - User's Manual:
   https://www.renesas.com/en/document/mat/ek-ra4c1-users-manual

.. _RA4C1 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/ra4c1-group-users-manual-hardware
