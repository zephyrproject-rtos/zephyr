.. _xmc47_relax_kit:

INFINEON XMC47-RELAX-KIT
########################

Overview
********

The XMC4700 Relax Kit is designed to evaluate the capabilities of the XMC4700
Microcontroller. It is based on High performance ARM Cortex-M4F which can run
up to 144MHz.

.. image:: xmc47_relax_kit.jpg
   :align: center
   :alt: XMC47-RELAX-KIT

Features:
=========

* ARM Cortex-M4F XMC4700
* On-board Debug Probe with USB interface supporting SWD + SWO
* Virtual COM Port via Debug Probe
* USB (Micro USB Plug)
* 32 Mbit Quad-SPI Flash
* Ethernet PHY and RJ45 Jack
* 32.768 kHz RTC Crystal
* microSD Card Slot
* CAN Transceiver
* 2 pin header x1 and x2 with 80 pins
* Two buttons and two LEDs for user interaction

Details on the Relax Kit development board can be found in the `Relax Kit User Manual`_.

Supported Features
==================

The Relax Kit development board configuration supports the following hardware features:

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
| SPI       | on-chip    | spi                   |
+-----------+------------+-----------------------+
| GPIO      | on-chip    | gpio                  |
+-----------+------------+-----------------------+
| FLASH     | on-chip    | flash                 |
+-----------+------------+-----------------------+
| ADC       | on-chip    | adc                   |
+-----------+------------+-----------------------+
| DMA       | on-chip    | dma                   |
+-----------+------------+-----------------------+
| PWM       | on-chip    | pwm                   |
+-----------+------------+-----------------------+
| WATCHDOG  | on-chip    | watchdog              |
+-----------+------------+-----------------------+
| MDIO      | on-chip    | mdio                  |
+-----------+------------+-----------------------+
| ETHERNET  | on-chip    | ethernet              |
+-----------+------------+-----------------------+
| PTP       | on-chip    | ethernet              |
+-----------+------------+-----------------------+

More details about the supported peripherals are available in `XMC4700 TRM`_
Other hardware features are not currently supported by the Zephyr kernel.

Building and Flashing
*********************
Flashing
========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xmc47_relax_kit
   :goals: flash

Debugging
=========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xmc47_relax_kit
   :goals: debug

Step through the application in your debugger.

References
**********

.. _Relax Kit User Manual:
   https://www.infineon.com/dgdl/Infineon-Board_User_Manual_XMC4700_XMC4800_Relax_Kit_Series-UserManual-v01_04-EN.pdf?fileId=5546d46250cc1fdf01513f8e052d07fc

.. _XMC4700 TRM:
   https://www.infineon.com/dgdl/Infineon-ReferenceManual_XMC4700_XMC4800-UM-v01_03-EN.pdf?fileId=5546d462518ffd850151904eb90c0044
