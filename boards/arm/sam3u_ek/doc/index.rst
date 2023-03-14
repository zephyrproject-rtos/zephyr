.. _sam3u_ek:

SAM3U-EK
########

Overview
********

The SAM3U-EK is an evaluation kit for the Atmel SAM3U family of microcontrollers.

Hardware
********

- ATSAM3U4E ARM Cortex-M3 Processor
- 12 MHz crystal oscillator
- 32.768 kHz crystal oscillator
- 2 user LEDs
- 2 user push buttons
- 1 UART
- 3 USART

Supported Features
==================

The sam3u_ek board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| USART     | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+

Other hardware features are not currently supported by Zephyr.

The default configuration can be found in the Kconfig
:zephyr_file:`boards/arm/sam3u_ek/sam3u_ek_defconfig`.

Connections and IOs
===================

The `SAM3U-EK Evaluation Kit User Guide`_ has detailed information about board
connections.

Serial Port
===========

The ATSAM3U4E MCU has 1 UART and 3 USARTs, The UART0 is configured for the
console.

References
**********

.. target-notes::

.. _SAM3U-EK Evaluation Kit User Guide:
    https://ww1.microchip.com/downloads/en/DeviceDoc/doc6478.pdf
