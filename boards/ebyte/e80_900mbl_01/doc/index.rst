.. zephyr:board:: e80_900mbl_01

Overview
********

E80-xxxMBL-01 series evaluation kit is designed to help users quickly evaluate Ebyte's
new generation LoRa dual-band wireless module.
The board is equipped with a STMicroelectronics general-purpose MCU STM32F103C8T6,
and most of the pins are led out to the pin headers on both sides.
E80-900MBL-01 features a Ebyte E80-900M2213S module with a Semtech LR1121 modem and is configured
for High Frequency LoRa bands (868MHz, 915MHz, etc) and the 2.4GHz LoRa band.

Hardware
********
E80-900MBL-01 provides the following hardware components:

- STM32 microcontroller in LQFP48 package

- Three LEDs:

   - User LED 1 (LED1), User LED 2 (LED2), power LED (PWR)

- Three Buttons:

   - Reset Button (RST), User Button 1 (K1), User Button 2 (K2)

- CH340 USB to UART adapter.

- E80-900M2213S Module, featuring semtech LR1121, configured for 850-930 MHz and 2.4GHz LoRa.

More information about E80-900MBL-01 can be found here:

- `E80-900MBL-01 Page`_
- `E80-900MBL-01 Schematic`_
- `E80-900M2213S Page`_
- `STM32F103 reference manual`_
- `STM32F103 data sheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
------------

The on-board 8MHz crystal is used to produce a 72MHz system clock with PLL.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``e80_900mbl_01`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The ``e80_900mbl_01`` board uses the SWD debug port that is broken out to an header for flashing.

Flashing an application to E80-900MBL-01
----------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: e80_900mbl_01
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: e80_900mbl_01
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _E80-900MBL-01 Page:
   https://www.cdebyte.com/products/E80-900MBL-01/

.. _E80-900MBL-01 Schematic:
   https://www.cdebyte.com/pdf-down.aspx?id=3480

.. _E80-900M2213S Page:
   https://www.cdebyte.com/products/E80-900M2213S/

.. _STM32F103 reference manual:
   https://www.st.com/resource/en/reference_manual/cd00171190.pdf

.. _STM32F103 data sheet:
   https://www.st.com/resource/en/datasheet/stm32f103rc.pdf
