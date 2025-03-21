.. zephyr:board:: nucleo_wb05kz

Overview
********

The Nucleo WB05KZ board is a Bluetooth |reg| Low Energy wireless and ultra-low-power
board featuring an ARM Cortex |reg|-M0+ based STM32WB05KZV MCU, embedding a
powerful and ultra-low-power radio compliant with the Bluetooth® Low Energy
SIG specification v5.4.

More information about the board can be found on the `Nucleo WB05KZ webpage`_.

Hardware
********

Nucleo WB05KZ provides the following hardware components:

- STM32WB05KZV in VFQFPN32 package
- ARM |reg| 32-bit Cortex |reg|-M0+ CPU
- 64 MHz maximal CPU frequebct
- 192 KB Flash
- 24 KB SRAM

More information about STM32WB05KZV can be found here:

- `WB05KZ on www.st.com`_
- `STM32WB05 reference manual`_


Supported Features
==================

.. zephyr:board-supported-hw::

Bluetooh support
----------------

BLE support is enabled; however, to build a Zephyr sample using this board,
you first need to fetch the Bluetooth controller library into Zephyr as a binary BLOB.

To fetch binary BLOBs:

.. code-block:: console

   west blobs fetch hal_stm32

Connections and IOs
===================

Default Zephyr Peripheral Mapping:
----------------------------------

- USART1 TX/RX       : PA1/PB0 (ST-Link Virtual COM Port)
- BUTTON (B1)        : PA0
- BUTTON (B2)        : PB5
- BUTTON (B3)        : PB14
- LED (LD1/BLUE)     : PB1
- LED (LD2/GREEN)    : PB4
- LED (LD3/RED)      : PB2

For more details, please refer to the `Nucleo WB05KZ board User Manual`_.

Programming and Debugging
*************************

Nucleo WB05KZ board includes an ST-LINK-V3EC embedded debug tool interface.

Applications for the ``nucleo_w05kz`` board target can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details).

Flashing
========

The board is configured to be flashed using the west `STM32CubeProgrammer`_ runner,
so :ref:`it must be installed <stm32cubeprog-flash-host-tools>` beforehand.

Alternatively, OpenOCD can also be used to flash the board using the
``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd

Flashing an application to Nucleo WB05KZ
----------------------------------------

Connect the Nucleo WB05KZ to your host computer using the USB port,
then run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Now build and flash an application. Here is an example for
:zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_wb05kz
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! nucleo_wb05kz/stm32wb05


Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_wb05kz
   :maybe-skip-config:
   :goals: debug

.. _`Nucleo WB05KZ webpage`:
   https://www.st.com/en/evaluation-tools/nucleo-wb05kz.html

.. _`WB05KZ on www.st.com`:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wb05kz.html

.. _`STM32WB05 reference manual`:
   https://www.st.com/resource/en/reference_manual/rm0529-stm32wb05xz-ultralow-power-wireless-32bit-mcu-armbased-cortexm0-with-bluetooth-low-energy-and-24-ghz-radio-solution-stmicroelectronics.pdf

.. _`Nucleo WB05KZ board User Manual`:
   https://www.st.com/resource/en/user_manual/um3343-stm32wb05-nucleo64-board-mb1801-and-mb2032-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
