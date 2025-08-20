.. zephyr:board:: ai_m62_12f

Overview
********

Ai-M62-12F is a Wi-Fi 6 + BLE5.3 module developed by Shenzhen Ai-Thinker Technology
Co., Ltd. The module is equipped with BL616 chip as the core processor, supports Wi-Fi
802.11b/g/n/ax protocol and BLE protocol, and supports Thread protocol. The BL616 system
includes a low-power 32-bit RISC-V CPU with floating-point unit, DSP unit, cache and
memory, with a maximum dominant frequency of 320M.

Hardware
********

For more information about the Bouffalo Lab BL-60x MCU:

- `Bouffalo Lab BL61x MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `ai_m62_12f Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The WB2 (BL602) Development Board is configured to run at max speed (192MHz).

Serial Port
===========

The ``ai_m62_12f`` board uses UART0 as default serial port.  It is connected
to USB Serial converter and port is used for both program and console.


Programming and Debugging
*************************

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: ai_m62_12f
      :goals: build flash

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ screen /dev/ttyUSB0 115200

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Then, press and release RST button

   .. code-block:: console

      *** Booting Zephyr OS build v4.2.0 ***
      Hello World! ai_m62_12f/bl616c50q2i

Congratulations, you have ``ai_m62_12f`` configured and running Zephyr.


.. _Bouffalo Lab BL61x MCU Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL616_DS/en

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _ai_m62_12f Schematics:
   https://docs.ai-thinker.com/en/ai_m62/
