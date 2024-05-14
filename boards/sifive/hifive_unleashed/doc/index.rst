.. zephyr:board:: hifive_unleashed

Overview
********

The HiFive Unleashed is a development board with a SiFive FU540-C000
multi-core 64bit RISC-V SoC.

Programming and debugging
*************************

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

Flashing
========

Current version has not yet supported flashing binary to onboard Flash ROM.

This board has USB-JTAG interface and this can be used with OpenOCD.
Load applications on DDR and run as follows:

.. code-block:: console

   openocd -c 'bindto 0.0.0.0' \
           -f boards/riscv/hifive_unleashed/support/openocd_hifive_unleashed.cfg
   riscv64-zephyr-elf-gdb build/zephyr/zephyr.elf
   (gdb) target remote :3333
   (gdb) c

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
