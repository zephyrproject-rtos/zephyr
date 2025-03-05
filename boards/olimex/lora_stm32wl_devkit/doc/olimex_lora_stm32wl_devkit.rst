.. zephyr:board:: olimex_lora_stm32wl_devkit

Overview
********

LoRaWAN development kit based on Olimex BB-STM32WL module using the
STM32WLE5CCU6 MCU.

Hardware
********

The board has below hardware features:

- BB-STM32WL, 256KB Flash, 64KB RAM with external antenna
- Lithium battery connector 3V (does not include battery)
- UEXT connector for external sensors
- BME280 temperature, humidity, pressure sensor
- LDR resistor for lighting measurement
- IIS2MDCTR 3-axis magnetometer for smart parking
- GPIO connector for prototyping
- Low power design
- 1 User LED
- 1 user, 1 boot, and 1 reset push-button
- 32.768 kHz LSE crystal oscillator

More information about the board and the module can be found here:

- `LoRa-STM32WL-DevKit Repository`_
- `LoRa-STM32WL-DevKit page on OLIMEX website`_
- `BB-STM32WL Module website`_
- `STM32WLE5CC reference manual`_
- `STM32WLE5CC on www.st.com`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Applications for the ``olimex_lora_stm32wl_devkit`` board configuration can be built the
usual way (see :ref:`build_an_application`).

The board contains an on-board debug probe which implements the CMSIS-DAP
interface.

It can also be debugged and flashed with an external debug probe connected
to the SWD pins.

The built-in debug probe works with pyOCD, but requires installing an additional
pack to support the STM32WL:

.. code-block:: console

   $ pyocd pack --update
   $ pyocd pack --install stm32wl

Flashing an application
=======================

Connect the board to your host computer and build and flash an application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: olimex_lora_stm32wl_devkit
   :goals: build flash

If you're using devkit revision C or higher, you'll need to specify the
appropriate revision letter to enable the VDDIO supply to the UEXT1 connector and
CON1 pin header.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: olimex_lora_stm32wl_devkit@D
   :goals: build flash

Run a serial terminal to connect with your board. By default, ``usart1`` is
accessible via the built-in USB to UART converter.

.. code-block:: console

   $ picocom --baud 115200 /dev/ttyACM0

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: olimex_lora_stm32wl_devkit
   :maybe-skip-config:
   :goals: debug

On board revisions C or newer:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: olimex_lora_stm32wl_devkit@D
   :maybe-skip-config:
   :goals: debug

.. _LoRa-STM32WL-DevKit Repository:
   https://github.com/OLIMEX/LoRa-STM32WL-DevKIT

.. _LoRa-STM32WL-DevKit page on OLIMEX website:
   https://www.olimex.com/Products/IoT/LoRa/LoRa-STM32WL-DevKit/open-source-hardware

.. _BB-STM32WL Module website:
   https://www.olimex.com/Products/IoT/LoRa/BB-STM32WL/

.. _STM32WLE5CC on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wle5cc.html

.. _STM32WLE5CC datasheet:
   https://www.st.com/resource/en/datasheet/stm32wle5cc.pdf

.. _STM32WLE5CC reference manual:
   https://www.st.com/resource/en/reference_manual/dm00530369-stm32wlex-advanced-armbased-32bit-mcus-with-subghz-radio-solution-stmicroelectronics.pdf
