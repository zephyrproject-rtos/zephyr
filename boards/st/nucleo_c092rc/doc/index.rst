.. zephyr:board:: nucleo_c092rc

Overview
********
The STM32 Nucleo-64 development board, featuring the STM32C092RC MCU,
supports both Arduino and ST morpho connectivity,
and includes a CAN FD interface with an onboard transceiver.

More information about the board can be found at the `Nucleo C092RC website`_.

Hardware
********
Nucleo C092RC provides the following hardware components:

- STM32 microcontroller in 64-pin package featuring 256 Kbytes of Flash memory
  and 30 Kbytes of SRAM.

- Flexible board power supply:

  - USB VBUS or external source (3.3V, 5V, 7 - 12V)
  - Current consumption measurement (IDD)

- Five LEDs:

  - Two user LEDs (LD1, LD2), one power LED (LD3),
    one STLINK LED (LD4), and one USB power fault LED (LD5)

- Three push-buttons: USER, RESET, BOOT

- On-board ST-LINK/V2-1 debugger/programmer with SWD connector

- Board connectors:

  - Arduino* Uno V3 expansion connector
  - ST morpho extension pin header
  - CAN FD interface with on-board transceiver

More information about STM32C092RC can be found here:
`STM32C0x1 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Each of the GPIO pins can be configured by software as output (push-pull or open-drain), as
input (with or without pull-up or pull-down), or as peripheral alternate function. Most of the
GPIO pins are shared with digital or analog alternate functions. All GPIOs are high current
capable except for analog inputs.

Default Zephyr Peripheral Mapping:
----------------------------------

- CAN RX/TX/STBY: PD0/PD1/PD2
- I2C1 SCL/SDA : PB8/PB9 (Arduino I2C)
- LD1       : PA5
- LD2       : PC9
- SPI1 NSS/SCK/MISO/MOSI : PA15/PA5/PA6/PA7 (Arduino SPI)
- UART_1 TX/RX : PB6/PB7 (Arduino Serial)
- UART_2 TX/RX : PA2/PA3 (ST-Link Virtual COM Port)
- USER_PB : PC13


For more details please refer to `STM32 Nucleo-64 board User Manual`_.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Nucleo C092RC board includes an ST-LINK/V2-1 embedded debug tool interface.

Applications for the ``nucleo_c092rc`` board can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, an external JLink (Software and Documentation Package Version >= v8.12e)
can also be used to flash the board using the ``--runner`` option:

.. code-block:: console

   $ west flash --runner jlink


Flashing an application to Nucleo C092RC
----------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_c092rc
   :goals: build flash

You will see the LED blinking every second.

References
**********

.. target-notes::

.. _Nucleo C092RC website:
   https://www.st.com/en/evaluation-tools/nucleo-c092rc.html

.. _STM32C0x1 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0490-stm32c0-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32 Nucleo-64 board User Manual:
   https://www.st.com/resource/en/user_manual/um3353-stm32-nucleo64-board-mb2046-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
