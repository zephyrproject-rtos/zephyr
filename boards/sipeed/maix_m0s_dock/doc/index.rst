.. zephyr:board:: maix_m0s_dock

Overview
********

The M0S is a very small module measuring 10x11mm using the BL616 SoC as the main control chip, with
the following features:

- Tri-Mode Wireless: WiFi6 / BT 5.2 / Zigbee
- High Frequency: 320MHz default
- Ultra-low Power Consumption: Wifi6 low power consumption feature
- High speed USB: Support USB2.0 HS OTG, up to 480Mbps
- Rich peripheral ports: Support RGB LCD, DVP Camera, Ethernet RMII and SDIO
- Tiny Size: Place ceramic antenna on 10x11 mm tiny size, and route all IO out

The M0S Dock is a carrier board for the M0S module and pushes out some of its I/O pins.

Hardware
********

For more information about the Bouffalo Lab BL-61x MCU:

- `Bouffalo Lab BL61x MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `maix_m0s_dock Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The M0S (BL616) Dock Board is configured to run at maximum speed (320MHz)
and can be overclocked to 480 MHz.

Serial Port
===========

The ``maix_m0s_dock`` board uses UART0 as default serial port.  It is connected to the pins on the
side of the USB port.


Programming and Debugging
*************************

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
   application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: maix_m0s_dock
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

   Then, unpower and re-power the board.

   .. code-block:: console

      *** Booting Zephyr OS build v4.2.0 ***
      Hello World! maix_m0s_dock/bl616c50q2i

Congratulations, you have ``maix_m0s_dock`` configured and running Zephyr.


.. _Bouffalo Lab BL61x MCU Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL616_DS/en

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _maix_m0s_dock Schematics:
   https://wiki.sipeed.com/hardware/en/maixzero/m0s/m0s.html
