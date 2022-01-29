.. _mpfs250t-miv:

Microsemi MPFS250T Mi-V
######################

Overview
********

The Microsemi MPFS250T board is an PolarFire SoC FPGA based development board.
The E51 RISC-V CPU can be deployed on the ICICLE board.
More information can be found on
`Microsemi's website <https://www.microsemi.com/product-directory/embedded-processing/4406-cpus>`_.

Programming and debugging
*************************

Building
========

Applications for the ``mpfs250t`` board configuration can be built as usual
(see :ref:`build_an_application`):

.. zephyr-app-commands::
   :board: mpfs250t
   :goals: build

By default the application is located in LIM at address 0x08000000. To locate
the application in DDR at address 0x80000000, append the following to the 
build command line:

 -- -DDTC_OVERLAY_FILE=~/zephyrproject/zephyr/boards/riscv/mpfs250t/mpfs250t-ddr.overlay
 

Flashing
========

In order to upload the application to the device, you'll need OpenOCD and GDB
with RISC-V support.
You can get them as a part of SoftConsole SDK.
Download and installation instructions can be found on
`Microsemi's SoftConsole website
<https://www.microsemi.com/product-directory/design-tools/4879-softconsole>`_.

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
   -ex "set arch riscv:rv32" \
   -ex "set riscv use_compressed_breakpoints no" \
   -ex "load" <path_to_executable>

Debugging
=========

Refer to the detailed overview of :ref:`application_debugging`.
