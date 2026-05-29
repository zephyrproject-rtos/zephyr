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

To Flash to the device, the easiest way to proceed is to load the binary using a sdcard.
The binary on the sdcard can then be loaded by the bootloader at the designated address.

For the bootloader to be able to load the application, a payload file needs to be generated.
Please proceed as follows:

To get the hss-payload-generator tool, please clone the following repository and build the tool:

.. code-block:: bash

   git clone https://github.com/pic64gx/pic64gx-hart-software-services.git
   cd pic64gx-hart-software-services/tools/hss-payload-generator
   make

To generate a payload:

.. code-block:: bash

   ./hss-payload-generator -c config.yaml -v output.bin

A payload generator config file such as the following should be used to generate a compatible with the Hart Software Services (HSS).
https://github.com/pic64gx/pic64gx-hart-software-services

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

Then the output payload file needs to be copied to the sdcard (assuming the sdcard is mounted at /dev/sdx).

.. code-block:: bash

   sudo dd if=<output_payload_file> of=/dev/sdx

Debugging
=========

Please note that in most use cases, the application must be loaded in the external DDR memory.
And therefore DDR must be initialized before loading the application or debugging the application.

The way to proceed is to load the HSS firmware first, then load the application in DDR memory through
the HSS loader following the instructions in `Flashing`_.

Then proceed to debug the application as usual (ie: :ref:`application_debugging`.)
