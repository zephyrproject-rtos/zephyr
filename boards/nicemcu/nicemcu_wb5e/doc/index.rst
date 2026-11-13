.. zephyr:board:: nicemcu_wb5e

Overview
********

The NiceMCU WB5E is a small development board carrying a BK7258 module,
a triple-core wireless SoC. Zephyr runs on CPU0, an Arm China STAR-MC1 core
implementing the Armv8-M Mainline architecture and binary compatible with the
Cortex-M33.

Hardware
********

- Arm China STAR-MC1, single precision FPU
- 640 KB of on-chip SRAM
- 8 MB of external QSPI flash, memory-mapped at 0x02000000 and executed in
  place
- 16 MB of PSRAM

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``nicemcu_wb5e/bk7258/cpu0`` board target can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board's USB serial adapter is connected to the BootROM's download UART and
to the chip's reset. ``west flash`` writes ``zephyr.crc.bin``, the image with
the flash CRC words, at offset 0 over that adapter. Close any terminal program
on the port first.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nicemcu_wb5e/bk7258/cpu0
   :goals: build flash

`BKFIL <https://dl.bekencorp.com/tools/flash/>`__ can also write ``zephyr.crc.bin``, which
boots from offset 0.

Flash Partitions
================

Offsets are physical. A code partition's size is the code it holds; with its
CRC words it occupies 34 bytes of flash for every 32.

=====================  ========  =======  ===========================
Partition              Offset    Size     Contents
=====================  ========  =======  ===========================
``slot0_partition``    0x000000  2 MB     the running image
``slot1_partition``    0x220000  2 MB     reserved for a second image
``storage_partition``  0x440000  3840 KB  data
=====================  ========  =======  ===========================

References
**********

- `Armino SDK <https://github.com/bekencorp/bk_idk>`_
- `BKFIL and other Beken flash tools <https://dl.bekencorp.com/tools/flash/>`_
