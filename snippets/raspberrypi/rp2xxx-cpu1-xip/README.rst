.. _rp2xxx-cpu1-xip:

RP2040/RP2350 cpu1 snippet with execution from Flash (_rp2xxx-cpu1-xip)
#######################################################################

Overview
********

This snippet allows users to build Zephyr with the capability to boot the
secondary CPU (``cpu1``) from the primary CPU (``cpu0``). CPU1 code is to be
executed from Flash, so the CPU1 image must be built with
:kconfig:option:`CONFIG_XIP` enabled.

NOTE: The ``rp2xxx-cpu1-xip`` snippet is much slower than the
``rp2xxx-cpu1`` snippet because of cache trashing in the XIP cache.

.. note::

   Currently only the RP2350 Cortex-M33 core is supported. The RP2040 and
   the RP2350 RISC-V (Hazard3) core are not yet supported.
