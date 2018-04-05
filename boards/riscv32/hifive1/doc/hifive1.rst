.. _hifive1:

SiFive HiFive1
##############

Overview
********

The HiFive1 is an Arduino-compatible development board with
an FE310 RISC-V SoC.
More information can be found on
`SiFive's website <https://www.sifive.com/products/hifive1>`_.

Programming and debugging
*************************

Building
========

Applications for the ``HiFive1`` board configuration can be built as usual
(see :ref:`build_an_application`).
In order to build the application for ``HiFive1``, set the ``BOARD`` variable
to ``hifive1``.

.. code-block:: bash

   export BOARD="hifive1"

Flashing
========

In order to upload the application to the device, you'll need OpenOCD and GDB
with RISC-V support.
Download and installation instructions can be found in the
`SiFive's Freedom-E-SDK GitHub repository
<https://github.com/sifive/freedom-e-sdk>`_.

With the necessary tools installed, you can connect to the board using OpenOCD.
To establish an OpenOCD connection, switch to the
:file:`utils` directory and run:

.. code-block:: bash

   # assuming that the location of the openocd is in PATH
   sudo openocd -f ${SDK_PATH}/bsp/env/freedom-e300-hifive1/openocd.cfg


Leave it running, and in a different terminal, use GDB to upload the binary to
the board. Use the RISC-V GDB from SiFive's Freedom-E-SDK toolchain.

Before loading the binary image to the device, the device's flash protection
must be disabled. Here are the GDB terminal commands to connect to the device,
disable the flash protection, load the binary, and start it running:

.. code-block:: console

   gdb
   (gdb) set remotetimeout 240
   (gdb) target extended-remote localhost:3333
   (gdb) monitor reset halt
   (gdb) monitor flash protect 0 64 last off
   (gdb) load {path to repository}/build/zephyr/zephyr.elf
   (gdb) monitor resume

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

