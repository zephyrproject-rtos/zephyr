.. zephyr:board:: canbridge_g473

Overview
********

The CANBRIDGE G473 is a versatile development board designed for automotive and
industrial communication. Based on the high-performance STM32G473 MCU, it can
function as a standalone 3-channel CAN/CAN FD bridge or, when running the
Zephyr connectivity stack, as a 3-channel USB-to-CAN FD adapter supporting
data rates up to 5 Mbps. More information can be found on the `can-module.com website`_.

Hardware
********

The CANBRIDGE G473 is equipped with a high-performance STM32G473 microcontroller.
The board features galvanic isolation, status LEDs, and onboard LIN and RS422
interfaces for expanded connectivity (hardware design ready, currently non-activated in devicetree).

Default Zephyr Peripheral Mapping
=================================

- CAN_RX1 : PB8
- CAN_TX1 : PB9
- CAN_RX2 : PB5
- CAN_TX2 : PB6
- CAN_RX3 : PB4
- CAN_TX3 : PB3
- LIN_RX : PB11
- LIN_TX : PB10
- ledCAN1 : PC13
- ledCAN2 : PB7
- ledCAN3 : PA15
- ledSTAT : PA5
- USB_DN : PA11
- USB_DP : PA12
- RS422_TX : PA2
- RS422_RX : PA3
- SWDIO : PA13
- SWCLK : PA14
- NRST : PG10

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The FDCAN1, FDCAN2, and FDCAN3 peripherals are driven by PLLQ,
which has 80 MHz frequency.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and :ref:`application_run`
for more details). If flashing via USB DFU, short the ``BOOT0`` pins/jumper prior to powering
the board to enter the built-in ROM bootloader mode.

Alternatively, you can flash the board directly using an ST-LINK debuggers.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: canbridge_g473
   :goals: flash

References
**********

.. _can-module.com website:
   https://can-module.com

.. _STM32G473 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g473.html

.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
