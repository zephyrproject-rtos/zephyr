.. zephyr:board:: mcx_n5xx_evk

Overview
********

The MCX-N5XX-EVK is very similar to the :doc:`mcx_n9xx_evk` board,
please see that page for instructions. The main difference is that this
board has an MCXN547 instead of an MCXN947.

For more information about the MCX-N547 SOC and MCX-N5XX-EVK board, see:

- `MCX-N547 SoC Website`_
- `MCX-N5XX-EVK Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

.. include:: ../../common/mcxa-mcxn-power-management.rst.inc

CPU1 does not support independent system power management.

The ``wakeup-button`` alias selects SW2 on P1_3, routed to WUU external pin 7
with a falling-edge interrupt.

Only SRAMA, the first 32 KB at ``0x20000000``, survives Deep Power Down through
the VBAT RAM LDO. The :zephyr_file:`samples/boards/nxp/mcxn_a/s2ram/mcxn.overlay`
limits ``zephyr,sram`` to the 24 KB at ``0x20002000`` to avoid the boot ROM's
scratch area. The default CPU0 RAM allocation is not a suspend-to-RAM layout;
check that the entire retained working set fits before enabling this state.

.. _MCX-N547 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-n-series-microcontrollers/mcx-n94x-54x-highly-integrated-multicore-mcus-with-on-chip-accelerators-intelligent-peripherals-and-advanced-security:MCX-N94X-N54X

.. _MCX-N5XX-EVK Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/MCX-N5XX-EVK
