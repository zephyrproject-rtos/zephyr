.. _rp2xxx-cpu1:

RP2040/RP2350 cpu1 snippet with execution from SRAM (rp2xxx-cpu1)
##################################################################

Overview
********

This snippet allows users to build Zephyr with the capability to boot the
secondary CPU (``cpu1``) from the primary CPU (``cpu0``). CPU1 code is to be
executed from SRAM, so the CPU1 image must be built with
:kconfig:option:`CONFIG_XIP` disabled.

.. note::

   This snippet launches a Cortex-M33. Use :ref:`rp2xxx-cpu1-riscv` to launch
   an RP2350 Hazard3 core. The RP2040 is not yet supported.
