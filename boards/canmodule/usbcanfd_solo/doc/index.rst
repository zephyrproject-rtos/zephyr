.. zephyr:board:: usbcanfd_solo

Overview
********

The USBCANFD SOLO is a high-performance 1-channel USB to CAN FD adapter board supporting
data rates up to 8 Mbps. More information can be found on the `can-module.com website`_.

Hardware
********

The USBCANFD SOLO board is equipped with an STM32G431C8 microcontroller and features
galvanic isolation, status LEDs for the CAN channel, and software-configurable
boot settings for entering the built-in bootloader mode.

Default Zephyr Peripheral Mapping
=================================

.. rst-class:: rst-columns

- CAN_RX/BOOT0 : PB8
- CAN_TX : PB9
- ledRX : PA6
- ledTX : PA5
- USB_DN : PA11
- USB_DP : PA12
- SWDIO : PA13
- SWCLK : PA14
- NRST : PG10

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The FDCAN1 peripheral is driven by PLLQ, which has a frequency of 80 MHz.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and :ref:`application_run`
for more details).

Flashing
========

Flashing via USB DFU requires access to the built-in ROM bootloader. Since the board does not
have a physical ``BOOT0`` pin, entry into DFU mode must be managed via option bytes.
Use an external debugger (such as ST-LINK) to change the ``nswBoot0`` bit to ``1`` to enter DFU mode.
After flashing the firmware via USB DFU, you must change the ``nswBoot0`` bit back to ``0``
to ensure normal boot behavior from Flash memory.

Alternatively, you can flash the board directly using an ST-LINK debugger.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: usbcanfd_solo
   :goals: flash

References
**********

.. _can-module.com website:
   https://can-module.com

.. _STM32G431C8 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g431c8.html

.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
