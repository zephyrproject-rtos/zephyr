.. _canmv_k230:

Canaan CanMV-K230
#################

Overview
********

The Canaan CanMV-K230 is a development board with a Canaan Kendryte K230
64bit RISC-V SoC.

Programming and debugging
*************************

Zephyr binary can be loaded by U-Boot command in machine mode.

.. code-block:: console

   fatload mmc 1:1 0x0 zephyr.bin
   go 0x0

Applications for the ``canmv_k230`` board configuration can be built as
usual (see :ref:`build_an_application`) using the corresponding board name:

.. zephyr-app-commands::
   :board: canmv_k230
   :goals: build
