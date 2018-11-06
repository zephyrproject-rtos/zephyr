.. _arty_bm310:

CloudBEAR BM-310
################

Overview
********

The CloudBEAR BM-310 is CloudBEAR IP design for Artix-7 35T Arty FPGA.
More information about BM-310 can be found on
`CloudBEAR's website <https://www.cloudbear.ru/bm_310.html>`_.

Programming and debugging
*************************

Building
========

Applications for the BM-310 board can be built as usual
(see :ref:`build_an_application`), using the ``arty_bm310`` board configuration:

.. code-block:: bash

   export BOARD="arty_bm310"

Flashing
========

Upload your application to the BM-310 board using OpenOCD
and GDB with RISC-V support.
Download and installation instructions can be found in the
`RISC-V OpenOCD GitHub repository
<https://github.com/riscv/riscv-openocd>`_ and
`RISC-V GDB GitHub repository
<https://github.com/riscv/riscv-binutils-gdb>`_ respectively.

With the necessary tools installed, establish an OpenOCD connection
to the board using:

.. code-block:: bash

   # assuming that the location of the openocd is in PATH
   sudo openocd -f your-openocd.cfg


Leave the OpenOCD session running, and in a different terminal,
use GDB to connect to the device, load the binary, and run the sample:

.. code-block:: console

   gdb
   (gdb) target extended-remote localhost:3333
   (gdb) monitor reset halt
   (gdb) load {path to repository}/build/zephyr/zephyr.elf

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

