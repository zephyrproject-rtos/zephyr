.. _stm32h750b_dk_board:

ST STM32H750B Discovery Kit
###########################

Overview
********

The STM32H750B-DK Discovery kit is a complete demonstration and development
platform for Arm® Cortex®-M7 core-based STM32H750XBH6 microcontroller, with
128Kbytes of Flash memory and 1 Mbytes of SRAM.

The STM32H750B-DK Discovery kit is used as a reference design for user
application development before porting to the final product, thus simplifying
the application development.

The full range of hardware features available on the board helps users to enhance
their application development by an evaluation of all the peripherals (such as
USB OTG FS, Ethernet, microSD™ card, USART, CAN FD, SAI audio DAC stereo with
audio jack input and output, MEMS digital microphone, HyperRAM™,
Octo-SPI Flash memory, RGB interface LCD with capacitive touch panel, and others).
ARDUINO® Uno V3, Pmod™ and STMod+ connectors provide easy connection to extension
shields or daughterboards for specific applications.

STLINK-V3E is integrated into the board, as the embedded in-circuit debugger and
programmer for the STM32 MCU and USB Virtual COM port bridge. STM32H750B-DK board
comes with the STM32CubeH7 MCU Package, which provides an STM32 comprehensive
software HAL library as well as various software examples.

.. image:: img/stm32h750b_dk.png
     :align: center
     :alt: STM32H750B-DK

More information about the board can be found at the `STM32H750B-DK website`_.
More information about STM32H750 can be found here:

- `STM32H750 on www.st.com`_
- `STM32H750xx reference manual`_
- `STM32H750xx datasheet`_

Supported Features
==================

The current Zephyr stm32h750b_dk board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| QSPI NOR  | on-chip    | off-chip flash                      |
+-----------+------------+-------------------------------------+
| RTC       | on-chip    | rtc                                 |
+-----------+------------+-------------------------------------+


Other hardware features are not yet supported on Zephyr porting.

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/st/stm32h750b_dk/stm32h750b_dk_defconfig`

Pin Mapping
===========

For more details please refer to `STM32H750B-DK website`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_3 TX/RX : PB10/PB11 (ST-Link Virtual Port Com)
- LD1 : PJ2
- LD2 : PI13

System Clock
============

The STM32H750B System Clock can be driven by an internal or external oscillator,
as well as by the main PLL clock. By default, the System clock
is driven by the PLL clock at 480MHz. PLL clock is feed by a 25MHz high speed external clock.

Serial Port
===========

The STM32H750B Discovery kit has up to 6 UARTs.
The Zephyr console output is assigned to UART3 which connected to the onboard ST-LINK/V3.0. Virtual
COM port interface. Default communication settings are 115200 8N1.


Programming and Debugging
*************************

See :ref:`build_an_application` for more information about application builds.


Flashing
========

Connect the STM32H750B-DK to your host computer using the ST-LINK
USB port, then run a serial host program to connect with the board. For example:

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyACM0

You can then build and flash applications in the usual way.
Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h750b_dk
   :goals: build flash

You should see the following message in the serial host program:

.. code-block:: console

   $ Hello World! stm32h750b_dk


Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h750b_dk
   :goals: debug


.. _STM32H750B-DK website:
   https://www.st.com/en/evaluation-tools/stm32h750b-dk.html

.. _STM32H750 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h750-value-line.html

.. _STM32H750xx reference manual:
   https://www.st.com/resource/en/reference_manual/rm0433-stm32h742-stm32h743753-and-stm32h750-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32H750xx datasheet:
   https://www.st.com/resource/en/datasheet/stm32h750ib.pdf
