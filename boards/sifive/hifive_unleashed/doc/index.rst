.. zephyr:board:: hifive_unleashed

Overview
********

The HiFive Unleashed is a development board with a SiFive FU540-C000
multi-core 64bit RISC-V SoC.

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Applications for the ``hifive_unleashed`` board configuration can be built as
usual (see :ref:`build_an_application`) using the corresponding board name:

.. tabs::

   .. group-tab:: E51

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: hifive_unleashed/fu540/e51
         :goals: build

   .. group-tab:: U54

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: hifive_unleashed/fu540/u54
         :goals: build

QEMU
====

The ``qemu`` variants of the board run on the ``sifive_u`` machine of QEMU, which emulates the
HiFive Unleashed: ``hifive_unleashed/fu540/e51/qemu`` on the E51 core, hart 0, and
``hifive_unleashed/fu540/u54/qemu`` on the U54 cores that follow it. The image is loaded into the
L2 LIM, as on the board.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: hifive_unleashed/fu540/u54/qemu
   :goals: build run

The machine has no QSPI1, so this device is disabled.

Flashing
========

Current version has not yet supported flashing binary to onboard Flash ROM.

This board has USB-JTAG interface and this can be used with OpenOCD.
Load applications on DDR and run as follows:

.. code-block:: console

   openocd -c 'bindto 0.0.0.0' \
           -f boards/sifive/hifive_unleashed/support/openocd_hifive_unleashed.cfg
   riscv64-zephyr-elf-gdb build/zephyr/zephyr.elf
   (gdb) target remote :3333
   (gdb) c

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
