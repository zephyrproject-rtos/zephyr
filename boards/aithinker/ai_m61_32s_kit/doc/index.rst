.. zephyr:board:: ai_m61_32s_kit

Overview
********

Ai-M61-32S is a Wi-Fi 6 + BLE5.3 module developed by Shenzhen Ai-Thinker Technology
Co., Ltd. The module is equipped with BL618 chip as the core processor, supports Wi-Fi
802.11b/g/n/ax protocol and BLE protocol, and supports Thread protocol. The BL618 system
includes a low-power 32-bit RISC-V CPU with floating-point unit, DSP unit, cache and
memory, with a maximum dominant frequency of 320M.

Hardware
********

For more information about the Bouffalo Lab BL-61x MCU:

- `Bouffalo Lab BL61x MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `ai_m61_32s_kit Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The M61 (BL618) Development Board is configured to run at maximum speed (320MHz)
and can be overclocked to 480 MHz.

This board provides demonstration configurations for the overclocking of the BL618 SoC clocks:

- ``ai_m61_32s_kit/bl618m05q2i/safe_overclock`` demonstrates 120MHz BCLK and 480MHz core clock on both variants. This nets a Coremark score up to 1600.
- ``ai_m61_32s_kit/bl618m05q2i/unsafe_overclock`` demonstrates 120MHz or 106MHz BCLK and 640MHz core clock with higher core voltages. This nets a Coremark score up to 2100.

If you are using the ALL variant, please use :``ai_m61_32s_kit@ALL``.

When overclocking, DMA and DMA-using drivers will not work properly.

Serial Port
===========

The ``ai_m61_32s_kit`` board uses UART0 as default serial port.  It is connected
to USB Serial converter and port is used for both program and console.


Programming and Debugging
*************************

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
   application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: ai_m61_32s_kit
      :goals: build flash

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ screen /dev/ttyUSB0 115200

   Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Then, press and release RST button

   .. code-block:: console

      *** Booting Zephyr OS build v4.3.0 ***
      Hello World! ai_m61_32s_kit/bl618m05q2i

Congratulations, you have ``ai_m61_32s_kit`` configured and running Zephyr.


.. _Bouffalo Lab BL61x MCU Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL616_DS/en

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _ai_m61_32s_kit Schematics:
   https://docs.ai-thinker.com/en/ai_m61/
