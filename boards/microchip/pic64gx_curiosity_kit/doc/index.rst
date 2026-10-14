.. zephyr:board:: pic64gx_curiosity_kit

Overview
********

The Microchip pic64gx_curiosity_kit board is a RISC-V based development board with a Microchip PIC64GX1000 SoC.
More information can be found on the `Microchip website <https://www.microchip.com/en-us/development-tool/curiosity-pic64gx1000-kit>`_.

Programming and debugging
*************************

Building
========

Applications for the ``pic64gx_curiosity_kit`` board configuration can be built as usual
(see :ref:`build_an_application`):

.. zephyr-app-commands::
   :board: pic64gx_curiosity_kit/pic64gx1000/u54
   :goals: build

.. zephyr-app-commands::
   :board: pic64gx_curiosity_kit/pic64gx1000/u54/smp
   :goals: build

Flashing
========

The board has an on-board Microchip debugger (embedded FlashPro5) on its USB-C debug port. It
provides JTAG access to the SoC and the serial consoles: ``MMUART0`` (used by the Hart Software
Services, HSS) on the second serial port of the debugger and ``MMUART1`` (the Zephyr console of
the ``u54`` targets) on the third.

The application runs from DDR memory, which is initialized by the HSS stored in the eNVM of the
SoC. The HSS must therefore be programmed in the eNVM, as on boards shipped by Microchip.

On Linux, OpenOCD needs access to the debugger. Add a udev rule such as the following to
:file:`/etc/udev/rules.d/70-microchip.rules` and replug the board:

.. code-block:: none

   SUBSYSTEM=="usb", ATTR{idVendor}=="1514", ATTR{idProduct}=="200a", MODE="0660", GROUP="plugdev", TAG+="uaccess"

Loading into DDR over JTAG
--------------------------

The default runner resets the SoC, waits for the HSS to initialize DDR and to boot whatever the
SD card holds, then halts the U54 harts, loads the application into DDR over JTAG and starts it.
The SD card can stay in the board. The application is lost on reset or power cycle.

.. zephyr-app-commands::
   :board: pic64gx_curiosity_kit/pic64gx1000/u54
   :goals: build flash

The time to wait for the HSS can be changed if needed (default 8000 ms):

.. code-block:: console

   west flash -- --cmd-pre-init "set HSS_BOOT_WAIT_MS 10000"

Writing to the SD card through the HSS
--------------------------------------

The ``mchp_hss`` runner stores the application on the SD card, so the HSS boots it on every
reset. It converts :file:`zephyr.elf` into an HSS payload with ``hss-payload-generator``, stops
the HSS in its command line interface on ``MMUART0``, sends the payload with YMODEM and lets the
HSS write it to the SD card.

.. warning::

   The HSS writes the payload to the start of the SD card. This destroys the partition table
   and any data on the card, for example a Linux image.

Get ``hss-payload-generator`` from the `PIC64GX HSS releases`_ or build it from the
`PIC64GX HSS`_ sources (:file:`tools/hss-payload-generator`), then either add it to ``PATH`` or
set the ``HSS_PAYLOAD_GENERATOR`` environment variable to its path. Close any terminal using the
``MMUART0`` port, then run:

.. code-block:: console

   west flash -r mchp_hss

The serial port of the on-board debugger is detected automatically, use ``--port`` to select
another one. ``--receive-only`` transfers and checks the payload without writing it.

Writing the SD card on a host
-----------------------------

The payload can also be written to the SD card on a host computer. To get the
hss-payload-generator tool, clone the following repository and build the tool:

.. code-block:: bash

   git clone https://github.com/pic64gx/pic64gx-hart-software-services.git
   cd pic64gx-hart-software-services/tools/hss-payload-generator
   make

To generate a payload:

.. code-block:: bash

   ./hss-payload-generator -c config.yaml -v output.bin

A payload generator configuration file such as the following generates a payload compatible
with the HSS.

For single core:

.. code-block:: yaml

   set-name: 'Zephyr-DDR'

   hart-entry-points: {
      u54_1: '0x80000000'
   }

   payloads:
      build/zephyr/zephyr.elf:    {
         exec-addr: '0x80000000',
         owner-hart: u54_1,
         priv-mode: prv_m,
         skip-opensbi: true,
         payload-name: "zephyr"
      }

For SMP:

.. code-block:: yaml

   set-name: 'Zephyr-SMP-DDR'

   hart-entry-points: {
      u54_1: '0x80000000',
      u54_2: '0x80000000',
      u54_3: '0x80000000',
      u54_4: '0x80000000'
   }

   payloads:
      build/zephyr/zephyr.elf:    {
         exec-addr: '0x80000000',
         owner-hart: u54_1,
         secondary-hart: u54_2,
         secondary-hart: u54_3,
         secondary-hart: u54_4,
         priv-mode: prv_m,
         skip-opensbi: true,
         payload-name: "zephyr"
      }

Please refer to the following README.md for more information on payload generation:
https://github.com/pic64gx/pic64gx-hart-software-services/blob/pic64gx/tools/hss-payload-generator/README.md

Then copy the payload file to the SD card (assuming the SD card is :file:`/dev/sdx`):

.. code-block:: bash

   sudo dd if=<output_payload_file> of=/dev/sdx

Debugging
=========

Load and start the application with ``west flash``, then attach the debugger to the running
application:

.. zephyr-app-commands::
   :board: pic64gx_curiosity_kit/pic64gx1000/u54
   :goals: attach

See :ref:`application_debugging` for more information.

.. _PIC64GX HSS:
   https://github.com/pic64gx/pic64gx-hart-software-services

.. _PIC64GX HSS releases:
   https://github.com/pic64gx/pic64gx-hart-software-services/releases
