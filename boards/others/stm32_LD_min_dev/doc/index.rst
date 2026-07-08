.. zephyr:board:: stm32_LD_min_dev

Overview
********

The STM32 Low Density Minimum Development Board is a low-cost variant of the
popular breadboard-friendly `STM32 Minimum Development Board`_ family, built
around the `STM32F103C6Tx`_ CPU instead of the STM32F103C8 used on the
original board. The STM32F103C6 is a low-density performance-line part: it
shares the same LQFP48 pinout and peripheral set as the STM32F103C8, but has
half the Flash (32 KB) and RAM (10 KB). Boards using this SoC are commonly
sold as cheaper "Blue Pill"-style clones and are a popular choice when the
smaller memory footprint is not a limiting factor for the application.

Zephyr applications can use the ``stm32_LD_min_dev`` board configuration to
target this board.

As the name suggests, this board has the bare minimum components required to
power on the CPU. For practical use, you'll need to add additional
components and circuits using a breadboard.

Hardware
********

This port is a starting point for your own customizations and not a complete
port for a specific board. Most of the GPIOs on the STM32 SoC have been
exposed in the external header with silk screen labels that match the SoC's
pin names.

Each board vendor has their own variations in pin mapping on their boards'
external connectors and placement of components. Many vendors use pin PC13
for connecting an LED, so only this device is supported by our Zephyr port.
Additional device support is left for the user to implement.

More information on hooking up peripherals and lengthy how-to articles can
be found at `EmbedJournal`_.

Boot Configuration
===================

The boot configuration for this board is set through jumpers on B0 (Boot 0)
and B1 (Boot 1). The B0 and B1 pins sit between the logic 0 and logic 1
lines; the silkscreen reads BX- or BX+ to indicate the 0 and 1 logic lines
for B0 and B1 respectively.

+--------+--------+--------------------+------------------------------------+
| Boot 1 | Boot 0 | Boot Mode          | Aliasing                            |
+========+========+====================+======================================+
| X      | 0      | Main Flash Memory  | Main flash memory is selected as    |
|        |        |                    | boot space                          |
+--------+--------+--------------------+--------------------------------------+
| 0      | 1      | System Memory      | System memory is selected as boot   |
|        |        |                    | space                                |
+--------+--------+--------------------+--------------------------------------+
| 1      | 1      | Embedded SRAM      | Embedded SRAM is selected as boot   |
|        |        |                    | space                                |
+--------+--------+--------------------+--------------------------------------+

System Clock
============

The on-board 8 MHz crystal is used to produce a 72 MHz system clock with the
PLL.

Serial Port
===========

The STM32 Low Density Minimum Development Board has 2 U(S)ARTs. The Zephyr
console output is assigned to UART_1. Default settings are 115200 8N1.

On-Board LED
============

The board has one on-board LED connected to PC13.

Supported Features
===================

.. zephyr:board-supported-hw::

Connections and IOs
====================

Default Zephyr Peripheral Mapping
----------------------------------

- UART_1 TX/RX: PA9/PA10
- UART_2 TX/RX: PA2/PA3
- I2C_1 SCL/SDA: PB6/PB7
- PWM_1_CH1: PA8
- SPI_1 NSS_OE/SCK/MISO/MOSI: PA4/PA5/PA6/PA7
- USB_DC DM/DP: PA11/PA12
- ADC_1: PA0

STLinkV2 Connection
====================

The board can be flashed using an STLinkV2 with the following connections.

+--------+---------------+
| Pin    | STLINKv2      |
+========+===============+
| G      | GND           |
+--------+---------------+
| CLK    | Clock         |
+--------+---------------+
| IO     | SW IO         |
+--------+---------------+
| V3     | VCC           |
+--------+---------------+

Programming and Debugging
**************************

.. zephyr:board-supported-runners::

Applications for the ``stm32_LD_min_dev`` board configuration can be built
and flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32_LD_min_dev
   :goals: build flash

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32_LD_min_dev
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _STM32F103C6Tx:
   https://www.st.com/resource/en/datasheet/stm32f103c6.pdf

.. _STM32 Minimum Development Board:
   https://docs.zephyrproject.org/latest/boards/others/stm32_min_dev/doc/index.html

.. _EmbedJournal:
   https://embedjournal.com/tag/stm32-min-dev/
