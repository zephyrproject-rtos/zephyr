.. zephyr:board:: eval_adema_apph5

Overview
********
The ADI EVAL-ADEMA-APPH5 is an evaluation board featuring the STM32H573ZI MCU.

The Zephyr port is running on the STM32H573ZI MCU.

Hardware
********

- STM32H573ZI MCU:

  - High-performance Arm Cortex-M33 core with TrustZone
  - 250MHz operating frequency
  - 2MB Flash, 640KB SRAM
  - Rich set of peripherals including GPIO, SPI, UART, I2C, DMA, PWM, Watchdog

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========
The STM32H573ZI MCU can be flashed by connecting an external debug probe to the
SWD port.

Once the debug probe is connected to your host computer, then you can simply run the
``west flash`` command to write a firmware image into flash.

Debugging
=========
Please refer to the `Flashing`_ section and run the ``west debug`` command
instead of ``west flash``.

References
**********

- `EVAL-ADEMA-APPH5 web page`_

.. _EVAL-ADEMA-APPH5 web page:
   https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/EVAL-ADEMA127.html
