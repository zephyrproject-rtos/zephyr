.. zephyr:board:: mc1board

Overview
********

The MC1 board is designed by Zurich University of Applied Sciences to train students in the field of embedded systems.
Some highlights of the MC1 board:

- STM32H573IIK3Q microcontroller featuring 2 MiB of Flash memory and 640 KiB of SRAM in 144-pin LQFP package
- 2.4-inch 240x320 pixels TFT-LCD with LED backlight and touch panel
- USB Type-C |trade| Host and device with USB power-delivery controller
- 10/100-Mbit/s Ethernet,
- microSD  |trade|
- Board connectors
  - Three male and two female 16-pin (2x8, 2.54 mm pitch) expansion headers
- Flexible power-supply options
  - J-Link USB
  - USER USB
- On-board J-Link debugger/programmer
- 4 user LEDs
- 4 user and one reset button
- 4 user slide switches

Hardware
********

The STM32H573xx devices are an high-performance microcontrollers family (STM32H5
Series) based on the high-performance Arm |reg| Cortex |reg|-M33 32-bit RISC core.
They operate at a frequency of up to 250 MHz.

More information about STM32H573 can be found here:

- `STM32H573 on www.st.com`_
- `STM32H573 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

MC1 board has 8 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_4 RX/TX : PB8/PB9 (VCP)
- USB DM/DP : PA11/PA12
- LED0 (green) : PF0
- LED1 (green) : PF1
- LED2 (green) : PF2
- LED3 (green) : PF3
- T0 : PF4
- T1 : PF5
- T2 : PF10
- T3 : PF11
- SW0 : PF12
- SW1 : PF13
- SW2 : PF14
- SW3 : PF15

System Clock
------------

STM32H573 System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
240MHz, driven by 25MHz external oscillator (HSE).

Serial Port
-----------

The Zephyr console output is assigned to UART4. Default settings are 115200 8N1.

TFT LCD screen and touch panel
------------------------------

The TFT LCD screen and touch panel are supported for the MC1 board.
They can be tested using :zephyr:code-sample:`lvgl` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: mc1board
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

MC1 board includes an J-Link embedded debug tool interface.

Applications for the ``mc1board`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).


Flashing
========

The board is configured to be flashed using west J-Link runner.

.. code-block:: console

  $ west flash --runner jlink

Flashing an application to MC1 board
------------------------------------

Connect the board to your host computer using the USB(J-Link) port.
Then build and flash an application. Here is an example for the
:zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mc1board
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! mc1board

Debugging
=========

Debugging could be performed with J-Link.
Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mc1board
   :maybe-skip-config:
   :goals: debug

References
**********

.. _STM32H573 on www.st.com:
   https://www.st.com/en/microcontrollers/stm32h573ii.html

.. _STM32H573 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0481-stm32h563h573-and-stm32h562-armbased-32bit-mcus-stmicroelectronics.pdf

