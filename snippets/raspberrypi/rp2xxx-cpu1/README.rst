.. _rp2xxx-cpu1:

RP2040/RP2350 cpu1 snippet with execution from SRAM (rp2xxx-cpu1)
##################################################################

Overview
********

This snippet allows users to build Zephyr with the capability to boot the
secondary CPU (``cpu1``) from the primary CPU (``cpu0``).  CPU1 code is to be
executed from SRAM, so the CPU1 image must be built with
:kconfig:option:`CONFIG_XIP` disabled.
