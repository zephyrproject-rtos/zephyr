.. zephyr:board:: hifive_unmatched

Overview
********

The HiFive Unmatched is a development board with a SiFive FU740-C000
multi-core 64bit RISC-V SoC.

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Applications for the ``hifive_unmatched`` board configuration can be built as
usual (see :ref:`build_an_application`) using the corresponding board name:

.. tabs::

   .. group-tab:: S7

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: hifive_unmatched/fu740/s7
         :goals: build

   .. group-tab:: U74

      .. zephyr-app-commands::
         :zephyr-app: samples/hello_world
         :board: hifive_unmatched/fu740/u74
         :goals: build

Flashing
========

Current version has not yet supported flashing binary to onboard Flash ROM.

This board has USB-JTAG interface and this can be used with OpenOCD.
Load applications on DDR and run as follows:

.. code-block:: console

   openocd -c 'bindto 0.0.0.0' \
           -f boards/riscv/hifive_unmatched/support/openocd_hifive_unmatched.cfg
   riscv64-zephyr-elf-gdb build/zephyr/zephyr.elf
   (gdb) target remote :3333
   (gdb) c

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
