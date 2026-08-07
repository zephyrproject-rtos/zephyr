.. zephyr:board:: bl618g0

Overview
********

The Bouffalo Lab BL618 G0 is an evaluation board based on the Bouffalo Lab BL618 SoC.
The BL618 is a Wi-Fi 6 + BLE 5.3 + IEEE 802.15.4 (Zigbee/Thread) capable RISC-V
microcontroller with a maximum clock frequency of 320 MHz and 480 KB of SRAM.

The board features:

- BL618M05Q2I Soc with BL618 core
- W25Q64 8 MB external SPI flash
- 32 MB embedded PSRAM
- On-board CK-Link debugger (no external probe needed)
- USB 2.0 HS OTG via USB-C
- MicroSD (TF) card slot
- IPEX antenna connector
- Expansion headers with all unused GPIOs broken out
- Support for optional expansion modules: I2S audio, SPI display, DVP camera
- One user LED (GPIO32)
- One user button (GPIO33)
- Reset and Boot buttons

Hardware
********

For more information about the Bouffalo Lab BL-61x MCU:

- `Bouffalo Lab BL61x MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `Bouffalo Lab BL618 G0 Board`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The BL618 G0 evaluation board is configured to run at maximum speed (320MHz).

Serial Port
===========

The ``bl618g0`` board uses UART0 as default serial port. It is connected
to the on-board CK-Link USB-to-UART interface and used for both flashing
and console output.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
   application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: bl618g0
      :goals: build flash

#. Run your favorite terminal program to listen for output:

   .. code-block:: console

      $ screen /dev/ttyACM0 115200

   Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Then, press and release RST button

   .. code-block:: console

      *** Booting Zephyr OS build v4.4.0 ***
      Hello World! bl618g0/bl618m05q2i

Congratulations, you have ``bl618g0`` configured and running Zephyr.


.. _Bouffalo Lab BL61x MCU Datasheet:
   https://github.com/bouffalolab/bl_docs/tree/main/BL616_DS/en

.. _Bouffalo Lab Development Zone:
   https://dev.bouffalolab.com/home?id=guest

.. _Bouffalo Lab BL618 G0 Board:
   https://verimake.com/d/260-bl618
