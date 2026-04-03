.. zephyr:board:: beaglev_fire

Overview
********

BeagleV®-Fire is a revolutionary single-board computer (SBC) powered by the Microchip’s
PolarFire® MPFS025T 5x core RISC-V System on Chip (SoC) with FPGA fabric. BeagleV®-Fire opens up new
horizons for developers, tinkerers, and the open-source community to explore the vast potential of
RISC-V architecture and FPGA technology. It has the same P8 & P9 cape header pins as BeagleBone
Black allowing you to stack your favorite BeagleBone cape on top to expand it’s capability.
Built around the powerful and energy-efficient RISC-V instruction set architecture (ISA) along with
its versatile FPGA fabric, BeagleV®-Fire SBC offers unparalleled opportunities for developers,
hobbyists, and researchers to explore and experiment with RISC-V technology.

Building
========

There are three board configurations provided for the BeagleV-Fire:

* ``beaglev_fire/polarfire/e51``: Uses only the E51 core
* ``beaglev_fire/polarfire/u54``: Uses the U54 cores
* ``beaglev_fire/polarfire/u54/smp``: Uses the U54 cores with CONFIG_SMP=y

Applications for the ``beaglev_fire`` board configuration can be built as usual:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: beaglev_fire/polarfire/u54
   :goals: build

Debugging
=========

In order to upload the application to the device, you'll need OpenOCD and GDB
with RISC-V support.
You can get them as a part of SoftConsole SDK.
Download and installation instructions can be found on
`Microchip's SoftConsole website
<https://www.microchip.com/en-us/products/fpgas-and-plds/fpga-and-soc-design-tools/programming-and-debug/softconsole>`_.

You will also require a Debugger such as Microchip's FlashPro5/6.

Connect to BeagleV-Fire UART debug port using a 3.3v USB to UART bridge.
Now you can run ``tio <port>`` in a terminal window to access the UART debug port connection. Once you
are connected properly you can press the Reset button which will show you a progress bar like:

.. image:: img/board-booting.png
     :align: center
     :alt: beaglev_fire

Once you see that progress bar on your screen you can start pressing any button (0-9/a-z) which
will interrupt the Hart Software Services from booting its payload.

With the necessary tools installed, you can connect to the board using OpenOCD.
from a different terminal, run:

.. code-block:: bash

   <softconsole_path>/openocd/bin/openocd --command "set DEVICE MPFS" --file \
   <softconsole_path>/openocd/share/openocd/scripts/board/microsemi-riscv.cfg


Leave it running, and in a different terminal, use GDB to upload the binary to
the board. You can use the RISC-V GDB from the Zephyr SDK.
launch GDB:

.. code-block:: bash

   <path_to_zephyr_sdk>/riscv64-zephyr-elf/bin/riscv64-zephyr-elf-gdb



Here is the GDB terminal command to connect to the device
and load the binary:

.. code-block:: bash

   set arch riscv:rv64
   set mem inaccessible-by-default off
   file <path_to_zehyr.elf>
   target extended-remote localhost:3333
   load
   break main
   continue

Flashing
========
When using the PolarFire `Hart Software Services <https://github.com/polarfire-soc/hart-software-services>`_ along with Zephyr, you need to use the `hss-payload-generator <https://github.com/polarfire-soc/hart-software-services/tree/master/tools/hss-payload-generator>`_ tool to generate an image that HSS can boot.

.. code-block:: yaml

  set-name: 'ZephyrImage'

  # Define the entry point address for each hart (U54 cores)
  hart-entry-points:
    u54_1: '0x1000000000'

  # Define the payloads (ELF binaries or raw blobs)
  payloads:
    <path_to_zephyr.elf>:
      exec-addr: '0x1000000000'  # Where Zephyr should be loaded
      owner-hart: u54_1  # Primary hart that runs Zephyr
      priv-mode: prv_m  # Start in Machine mode
      skip-opensbi: true  # Boot directly without OpenSBI

After generating the image, you can flash it to the board by restarting a board that's connected over USB and UART, interrupting the HSS boot process with a key press, and then running the ``mmc`` and ``usbdmsc`` commands:

.. code-block:: bash

  Press a key to enter CLI, ESC to skip
  Timeout in 1 second
  .[6.304162] Character 100 pressed
  [6.308415] Type HELP for list of commands
  [6.313276] >> mmc
  [10.450867] Selecting SDCARD/MMC (fallback) as boot source ...
  [10.457550] Attempting to select eMMC ... Passed
  [10.712708] >> usbdmsc
  [14.732841] initialize MMC
  [14.736400] Attempting to select eMMC ... Passed
  [15.168707] MMC - 512 byte pages, 512 byte blocks, 30621696 pages
  Waiting for USB Host to connect... (CTRL-C to quit)
  . 0 bytes written, 0 bytes read
  USB Host connected. Waiting for disconnect... (CTRL-C to quit)
  / 0 bytes written, 219136 bytes read

This will cause the board to appear as a USB mass storage device. You can then then flash the image with ``dd`` or other tools like `BalenaEtcher <https://www.balena.io/etcher/>`_:

.. code-block:: bash

  dd if=<path_to_zephyr.elf> of=/dev/sdXD bs=4M status=progress oflag=sync
