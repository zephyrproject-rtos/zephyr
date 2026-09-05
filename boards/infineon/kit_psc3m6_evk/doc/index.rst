.. zephyr:board:: kit_psc3m6_evk

PSOC™ Control C3M6 Evaluation Kit
###################################

Overview
********

The PSOC™ Control C3M6 Evaluation Kit (KIT_PSC3M6_EVK) is an evaluation board for the
PSC3M6 microcontroller. The PSC3M6 is an Arm® Cortex®-M33 based SoC from Infineon's
CAT1B family, featuring 512 KB flash, 128 KB SRAM, and TrustZone-M security.

Hardware
********

- PSC3M6GES3AHQ1 (PG-E-LQFP-100) microcontroller
- Cortex-M33 core at up to 180 MHz (250 MHz DPLL250)
- 512 KB internal flash
- 128 KB SRAM
- 2 user LEDs (P8.4, P8.5)
- 2 user buttons (P10.2, P2.0)
- Debug UART via SCB3 (P6.3 TX, P6.2 RX)
- CAN FD (CANFD0 channel 1: P5.2 RX, P5.3 TX)
- I2C via SCB0 (P9.0 SCL, P9.2 SDA)
- SPI via SCB2 (P7.0 CLK, P7.1 MOSI, P7.2 MISO)
- OpenOCD debug/flash via SEGGER J-Link (requires SEGGER J-Link version v9.68 or later)

Supported Features
******************

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Use ``west flash`` and ``west debug`` with OpenOCD and SEGGER J-Link probe (requires SEGGER J-Link version v9.68 or later).

Infineon OpenOCD Installation
=============================

The `ModusToolbox™ Programming Tools`_ package includes Infineon OpenOCD.
Alternatively, a standalone installation can be done by downloading the
`Infineon OpenOCD`_ release for your system and extracting the files to a
location of your choice.

Flashing
========

Build and flash the application:

.. code-block:: shell

   west build -b kit_psc3m6_evk -p always samples/hello_world
   west flash --openocd <path/to/openocd>

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest
