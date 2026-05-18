.. zephyr:board:: dnn647

Overview
********

The DNN647 is an Alientek development board based on STM32N647X0H3Q.
This first Zephyr board port provides basic bring-up support:

- USART1 console (PE5/PE6)
- User LEDs (PG10, PE10)
- User button WK_UP (PC13)

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Build example:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: dnn647
   :goals: build

Serial boot variant:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: dnn647//sb
   :goals: build

References
**********

- `Alientek DNN647 documentation <https://wiki.alientek.com/docs/Boards/STM32/DNN647/start-guide/stm32n647-board-introduction/>`_
