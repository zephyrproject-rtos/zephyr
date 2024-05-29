.. zephyr:board:: nucleo_n657x0_q

Overview
********

The STM32 Nucleo-144 board provides an affordable and flexible way for users to try out
new concepts and build prototypes by choosing from the various combinations of performance
and power consumption features, provided by the STM32 microcontroller. For the compatible boards,
the internal or external SMPS significantly reduces power consumption in Run mode.

The ST Zio connector, which extends the ARDUINO® Uno V3 connectivity, and the ST morpho headers
provide an easy means of expanding the functionality of the Nucleo open development platform with
a wide choice of specialized shields.

The STM32 Nucleo-144 board does not require any separate probe as it integrates the ST-LINK
debugger/programmer.

The STM32 Nucleo-144 board comes with the STM32 comprehensive free software libraries and
examples available with the STM32Cube MCU Package.

Hardware
********

- Common features:

  - STM32 microcontroller in an LQFP144, TFBGA225, or VFBGA264 package
  - 3 user LEDs
  - 1 user push-button and 1 reset push-button
  - 32.768 kHz crystal oscillator
  - Board connectors:

    - SWD
    - ST morpho expansion connector

  - Flexible power-supply options: ST-LINK USB VBUS, USB connector, or external sources

- Features specific to some of the boards (refer to the ordering information section
  of the data brief for details);

  - External or internal SMPS to generate Vcore logic supply
  - Ethernet compliant with IEEE-802.3-2002
  - USB Device only, USB OTG full speed, or SNK/UFP (full-speed or high-speed mode)
  - Board connectors:

    - ARDUINO® Uno V3 connector or ST Zio expansion connector including ARDUINO® Uno V3
    - Camera module FPC
    - MIPI20 compatible connector with trace signals
    - USB with Micro-AB or USB Type-C®
    - Ethernet RJ45

  - On-board ST-LINK (STLINK/V2-1, STLINK-V3E, or STLINK-V3EC) debugger/programmer with
    USB re-enumeration capability: mass storage, Virtual COM port, and debug port

Supported Features
==================

The Zephyr ``nucleo_n657x0_q`` board supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+


Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/st/nucleo_n657x0_q/nucleo_n657x0_q_defconfig`


Connections and IOs
===================

NUCLEO_N657X0_Q Board has 12 GPIO controllers. These controllers are responsible
for pin muxing, input/output, pull-up, etc.

For more details please refer to `NUCLEO_N657X0_Q User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- LD1 : PO1
- LD2 : PG10
- USART_1_TX : PE5
- USART_1_RX : PE6

System Clock
------------

NUCLEO_N657X0_Q System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
400MHz, driven by 64MHz high speed internal oscillator.

Serial Port
-----------

NUCLEO_N657X0_Q board has 10 U(S)ARTs. The Zephyr console output is assigned to
USART1. Default settings are 115200 8N1.

Programming and Debugging
*************************

NUCLEO_N657X0_Q board includes an ST-LINK/V3 embedded debug tool interface.
This probe allows to flash the board using various tools.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.
Version 2.18.0 or later of `STM32CubeProgrammer`_ is required.

Flashing an application to NUCLEO_N657X0_Q
------------------------------------------

Connect the NUCLEO_N657X0_Q to your host computer using the USB port.
Then build and flash an application.

.. note::
   For flashing, BOOT0 pin should be set to 0 and BOOT1 to 1 before powering on
   the board.

   To run the application after flashing, BOOT1 should be set to 0 and the board
   should be powered off and on again.

Here is an example for the :zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_n657x0_q
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! nucleo_n657x0_q/stm32n657xx

Debugging
=========

For now debugging is only available through STM32CubeIDE:
* Go to File > Import and select C/C++ > STM32 Cortex-M Executable
* In Executable field, browse to your <ZEPHYR_PATH>/build/zephyr/zephyr.elf
* In MCU field, select STM32N657X0HxQ.
* Click on Finish
* Then click on Debug to start the debugging session

.. note::
   For debugging, BOOT0 pin should be set to 0 and BOOT1 to 1 before powering on the
   board.

.. _NUCLEO_N657X0_Q website:
   https://www.st.com/en/evaluation-tools/nucleo-n657x0-q.html

.. _NUCLEO_N657X0_Q User Manual:
   https://www.st.com/resource/en/user_manual/um3417-stm32n6-nucleo144-board-mb1940-stmicroelectronics.pdf

.. _STM32N657X0 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32n657x0.html

.. _STM32N657 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0486-stm32n647657xx-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
