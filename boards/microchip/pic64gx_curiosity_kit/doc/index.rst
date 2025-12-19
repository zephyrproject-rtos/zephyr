.. _pic64gx_curiosity_kit:

Microchip pic64gx_curiosity_kit
#####################

Overview
********

The Microchip pic64gx_curiosity_kit board is a RISC-V based development board with a Microchip PIC64GX1000 SoC.
The E51 RISC-V CPU can be deployed on the pic64gx_curiosity_kit board.
More information can be found on the `Microchip website <https://www.microchip.com/en-us/product/PIC64GX1000>`_.

Programming and debugging
*************************

Building
========

Applications for the ``pic64gx_curiosity_kit`` board configuration can be built as usual
(see :ref:`build_an_application`):

.. zephyr-app-commands::
   :board: pic64gx_curiosity_kit
   :goals: build


Flashing
========

In order to upload the application to the device, you'll need OpenOCD and GDB
with RISC-V support.
You can get them as a part of SoftConsole SDK.
Download and installation instructions can be found on
`Microchip's SoftConsole website
<https://www.microchip.com/en-us/products/fpgas-and-plds/fpga-and-soc-design-tools/programming-and-debug/softconsole>`_.

With the necessary tools installed, you can connect to the board using OpenOCD.
To establish an OpenOCD connection run:

.. code-block:: bash

   sudo LD_LIBRARY_PATH=<softconsole_path>/openocd/bin \
   <softconsole_path>/openocd/bin/openocd  --file \
   <softconsole_path>/openocd/share/openocd/scripts/board/microsemi-riscv.cfg


Leave it running, and in a different terminal, use GDB to upload the binary to
the board. You can use the RISC-V GDB from a toolchain delivered with
SoftConsole SDK.

Here is the GDB terminal command to connect to the device
and load the binary:

.. code-block:: console

   <softconsole_path>/riscv-unknown-elf-gcc/bin/riscv64-unknown-elf-gdb \
   -ex "target extended-remote localhost:3333" \
   -ex "set mem inaccessible-by-default off" \
   -ex "set arch riscv:rv64" \
   -ex "set riscv use_compressed_breakpoints no" \
   -ex "load" <path_to_executable>

Debugging
=========

Refer to the detailed overview of :ref:`application_debugging`.
