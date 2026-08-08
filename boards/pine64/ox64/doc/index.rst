.. zephyr:board:: ox64

Overview
********

The Pine64 Ox64 is a breadboard-friendly (Rasperry Pi Pico Format) RISC-V single board computer
built around the Bouffalo Lab BL808 tri-core SoC. It ships in two flash variants, 16 MB and 2MB.
The 2MB variant has the SD slot unpopulated.

Hardware
********

- SoC: BL808 tri-core RISC-V

  - **M0**: T-Head E907 (RV32IMAFCP, 320 MHz) — *supported by Zephyr*
  - **D0**: T-Head C906 (RV64IMAFCV, 480 MHz) — *not yet supported*
  - **LP**: T-Head E902 (RV32EMC, 160 MHz) — *not yet supported*

- Flash: 16 MB (W25Q128JWSQ) or 2 MB (GD25LQ16E) XSPI NOR flash, depending on variant
- MicroSD slot: populated on the 16 MB variant only
- A boot mode selection button. Do not press while using the board! The pin is shared with the flash
- A power LED
- Console: UART0 at 115200 baud

For more information about the Bouffalo Lab BL808 SoC:

- `Bouffalo Lab BL808 Datasheet`_
- `Pine64 OX64 Wiki`_

Supported Features
===================

.. zephyr:board-supported-hw::

System Clock
============

The BL808 default clock configuration:

- M0 (E907): 320 MHz (WIFIPLL)
- D0 (C906): 480 MHz (CPUPLL) — *not yet supported*
- LP (E902): 160 MHz — *not yet supported*

Serial Port
===========

The ``ox64`` board uses UART0 (GPIO14/GPIO15, header pins 1/2) as the default
serial port and for flashing.

Programming and Debugging
**************************

Building
========

The board comes in two flash size variants, selected at build time:

.. code-block:: console

   # 16 MB flash, includes microSD support
   west build -b ox64 samples/hello_world

   # 2 MB flash
   west build -b ox64//2mb samples/hello_world

.. zephyr:board-supported-runners::

Flashing
========

#. Build and flash the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: ox64
      :goals: build flash

#. Run your favorite terminal program to listen for output. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB0 -o

   Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   .. code-block:: console

      *** Booting Zephyr OS build v4.4.0 ***
      Hello World! ox64/bl808c09q2i


.. _Bouffalo Lab BL808 Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL808_DS/en

.. _Pine64 OX64 Wiki:
	https://wiki.pine64.org/wiki/Ox64
