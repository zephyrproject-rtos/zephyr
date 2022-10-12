.. _xmc4800_automation_board_v2:

INFINEON XMC4800-AUTOMATION-BOARD-V2
####################################

Overview
********

The XMC4800 Automation Board is designed to evaluate the capabilities of the XMC4800
Microcontroller. It is based on High performance ARM Cortex-M4 which can run
up to 144MHz.

.. image:: xmc4800_automation_board_v2.jpg
   :align: center
   :alt: XMC4800-AUTOMATION-BOARD-V2

Features:
=========

* ARM Cortex-M4 XMC4800
* 2048kB Flash Memory
* 352kB SRAM and 8MB SDRAM
* CAN transceiver and CAN connector (D-Sub DE-9)
* 24V ISOFACE™ 8 x IN and 8 x OUT
* USB Interface (Micro-AB USB plug)
* General purpose CAN LED, RGB user LED
* Reset Push-Button
* 12 MHz Crystal
* 32.768 kHz RTC Crystal

Details on the evaluation board can be found in the `Automation Board User Manual`_.

Supported Features
==================

* The on-board 12-MHz crystal allows the device to run at its maximum operating speed of 144MHz.

The development board configuration supports the following hardware features:

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| NVIC      | on-chip    | nested vectored       |
|           |            | interrupt controller  |
+-----------+------------+-----------------------+
| SYSTICK   | on-chip    | system clock          |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port           |
+-----------+------------+-----------------------+
| GPIO      | on-chip    | gpio                  |
+-----------+------------+-----------------------+
| SPI       | on-chip    | spi                   |
+-----------+------------+-----------------------+
| FLASH     | on-chip    | flash                 |
+-----------+------------+-----------------------+
| ADC       | on-chip    | adc                   |
+-----------+------------+-----------------------+
| DMA       | on-chip    | dma                   |
+-----------+------------+-----------------------+

Building and Flashing
*********************
Flashing
========

Here is an example for the :ref:`blinky-sample` application. The UART connection
is available on the 6 Pin header (pins 2 and 4).

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: xmc4800_automation_board_v2
   :goals: flash

Debugging
=========

Here is an example for the :ref:`blinky-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: xmc4800_automation_board_v2
   :goals: debug

Step through the application in your debugger.

References
**********

.. _Automation Board User Manual:
   https://www.infineon.com/dgdl/Infineon-XMC4800_Automation_Board-V2-UM-v01_00-EN.pdf?fileId=5546d4625a888733015ab7d86b5767cd
