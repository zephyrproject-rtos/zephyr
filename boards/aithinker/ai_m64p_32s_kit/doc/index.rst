.. zephyr:board:: ai_m64p_32s_kit

Overview
********

Ai-M64P-32S is a Wi-Fi 6 + BLE5.3 module developed by Shenzhen Ai-Thinker Technology
Co., Ltd. The module is equipped with the BL616CL chip as the core processor, supports Wi-Fi
802.11b/g/n/ax protocol and BLE protocol, and supports Thread protocol. The BL616CL system
includes a low-power, high-performance 32-bit RISC-V CPU with floating-point unit,
DSP unit (P extension), 16KB ICache, 8KB DCache, and 388KB of memory.

Hardware
********

Ai-M64P-32S-Kit provides the following hardware components:

- Ai-M64P-32S module with BL616CL, and two internal buck regulators controlled by GPIO2 and GPIO3

- Five total LEDs:

   - RGB LEDs on GPIO14, 15, and 16
   - Warm White LED on GPIO28
   - Cold White LED on GPIO27

- Two Buttons:

   - Reset Button (EN)
   - Boot Select Button (BOOT)

- CH340C USB to UART adapter.

For more information about the Bouffalo Lab BL616CL MCU:

- `Bouffalo Lab BL616CL MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `ai_m64p_32s_kit Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The M64P (BL616CL) Development Board is configured to run at maximum speed (320MHz).

Serial Port
===========

The ``ai_m64p_32s_kit`` board uses UART0 as default serial port.  It is connected
to USB Serial converter and port is used for both program and console.


Programming and Debugging
*************************

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
   application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: ai_m64p_32s_kit
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

      *** Booting Zephyr OS build v4.4.0 ***
      Hello World! ai_m64p_32s_kit/bl616cl4855

Congratulations, you have ``ai_m64p_32s_kit`` configured and running Zephyr.


.. _Bouffalo Lab BL616CL MCU Datasheet:
	https://aithinker-static.oss-cn-shenzhen.aliyuncs.com/docs/media/WiFi/M64/M64P/Datasheets/BL616CL_DS_en_0.9.4.pdf

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _ai_m64p_32s_kit Schematics:
   https://docs.ai-thinker.com/en/ai_m64/
