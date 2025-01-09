.. zephyr:board:: nucleo_wb07cc

Overview
********

The Nucleo WB07CC board is a Bluetooth |reg| Low Energy wireless and ultra-low-power
board featuring an ARM Cortex |reg|-M0+ based STM32WB07CCV MCU, embedding a
powerful and ultra-low-power radio compliant with the Bluetooth |reg| Low Energy
SIG specification v5.4.

More information about the board can be found on the `Nucleo WB07CC webpage`_.

Hardware
********

Nucleo WB07CC provides the following hardware components:

- STM32WB07CCV in VFQFPN32 package
- ARM |reg| 32-bit Cortex |reg|-M0+ CPU
- 64 MHz maximal CPU frequency
- 256 KB Flash
- 64 KB SRAM

More information about STM32WB07CCV can be found here:

- `WB07CC on www.st.com`_
- `STM32WB07 reference manual`_


Supported Features
==================

The Zephyr ``nucleo_wb07cc`` board target supports the following hardware features:

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
| FLASH     | on-chip    | internal flash memory               |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+
| TIMER     | on-chip    | counter, pwm                        |
+-----------+------------+-------------------------------------+
| RADIO     | on-chip    | Bluetooth Low Energy                |
+-----------+------------+-------------------------------------+


Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/st/nucleo_wb07cc/nucleo_wb07cc_defconfig`

Bluetooth support
-----------------

BLE support is enabled; however, to build a Zephyr sample using this board,
the Bluetooth controller library must be fetched into Zephyr as a binary blob.

To fetch the binary blobs:

.. code-block:: console

   $ west blobs fetch hal_stm32

Connections and IOs
===================

Default Zephyr Peripheral Mapping:
----------------------------------

- USART1 TX/RX       : PA9/PA8 (ST-Link Virtual COM Port)
- BUTTON (B1)        : PA0
- BUTTON (B2)        : PB5
- BUTTON (B3)        : PB9
- LED (LD1/BLUE)     : PB0
- LED (LD2/GREEN)    : PB4
- LED (LD3/RED)      : PB2

For more details, please refer to the `Nucleo WB07CC board User Manual`_.

Programming and Debugging
*************************

Nucleo WB07CC board includes an ST-LINK-V3EC embedded debug tool interface.

Applications for the ``nucleo_wb07cc`` board target can be built and flashed
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

Flashing an application to Nucleo WB07CC
----------------------------------------

Connect the Nucleo WB07CC to your host computer using the USB port,
then run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Now build and flash an application. Here is an example for
:zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_wb07cc
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! nucleo_wb07cc/stm32wb07


Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_wb07cc
   :maybe-skip-config:
   :goals: debug

.. _`Nucleo WB07CC webpage`:
   https://www.st.com/en/evaluation-tools/nucleo-wb07cc.html

.. _`WB07CC on www.st.com`:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wb07cc.html

.. _`STM32WB07 reference manual`:
   https://www.st.com/resource/en/reference_manual/rm0530--stm32wb07xc-and-stm32wb06xc-ultralow-power-wireless-32bit-mcus-armbased-cortexm0-with-bluetooth-low-energy-and-24-ghz-radio-solution-stmicroelectronics.pdf

.. _`Nucleo WB07CC board User Manual`:
   https://www.st.com/resource/en/user_manual/um3344-stm32wb07-nucleo64-board-mb1801-and-mb2119-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
