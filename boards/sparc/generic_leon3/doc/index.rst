.. _generic_leon3:

Generic LEON3
#############

Overview
********

This board configuration is designed to work with LEON3 processor template
designs available in the GRLIB GPL distribution.
It can also be used with the evaluation version of the TSIM3 LEON3 simulator.

Hardware
********

The board configuration is compatible with most GRLIB LEON3 FPGA template
designs such as the Digilent Arty A7, Terasic DE0-Nano and Microsemi
M2GL-EVAL-KIT.

Programming and Debugging
*************************

Building
========

Applications for the ``generic_leon3`` board configuration can be built as usual
(see :ref:`build_an_application`).
In order to build the application for ``generic_leon3``, set the ``BOARD`` variable
to ``generic_leon3``.

Running on hardware
===================

Connect with GRMON, then load and run the application. The example below uses
the Terasic DE2-115 Cyclone IV FPGA board.

.. code-block:: console

    $ grmon -altjtag -u
      GRMON debug monitor v3.2.8 eval version

      Copyright (C) 2020 Cobham Gaisler - All rights reserved.
      For latest updates, go to http://www.gaisler.com/
      Comments or bug-reports to support@gaisler.com

    JTAG chain (1): EP3C120/EP4CE115
      GRLIB build version: 4250
      Detected frequency:  50.0 MHz

      Component                            Vendor
      LEON3 SPARC V8 Processor             Cobham Gaisler
      AHB Debug UART                       Cobham Gaisler
      JTAG Debug Link                      Cobham Gaisler
      GR Ethernet MAC                      Cobham Gaisler
      GRDMAC DMA Controller                Cobham Gaisler
      LEON2 Memory Controller              European Space Agency
      AHB/APB Bridge                       Cobham Gaisler
      LEON3 Debug Support Unit             Cobham Gaisler
      Generic UART                         Cobham Gaisler
      Multi-processor Interrupt Ctrl.      Cobham Gaisler
      Modular Timer Unit                   Cobham Gaisler
      General Purpose I/O port             Cobham Gaisler
      SPI Controller                       Cobham Gaisler
      AHB Status Register                  Cobham Gaisler

      Use command 'info sys' to print a detailed report of attached cores

    grmon3> load zephyr/zephyr.elf
          40000000 text              16.2kB /  16.2kB   [===============>] 100%
          400040A8 initlevel           40B              [===============>] 100%
          400040D0 rodata             484B              [===============>] 100%
          400042B4 datas               20B              [===============>] 100%
          400042C8 sw_isr_table       256B              [===============>] 100%
          400043C8 devices             36B              [===============>] 100%
      Total size: 16.98kB (1.91Mbit/s)
      Entry point 0x40000000
      Image zephyr/zephyr.elf loaded

    grmon3> run
    *** Booting Zephyr OS build zephyr-v2.4.0-30-ga124c31ec4cf  ***
    Hello World! generic_leon3


Running in simulation
=====================

The same application binary can be simulated with the TSIM3 LEON3 simulator.

.. code-block:: console

    $ tsim-leon3
     TSIM3 LEON3 SPARC simulator, version 3.0.2 (evaluation version)

     Copyright (C) 2020, Cobham Gaisler - all rights reserved.
     This software may only be used with a valid license.
     For latest updates, go to https://www.gaisler.com/
     Comments or bug-reports to support@gaisler.com

    Number of CPUs: 2
    system frequency: 50.000 MHz
    icache: 1 * 4 KiB, 16 bytes/line (4 KiB total)
    dcache: 1 * 4 KiB, 16 bytes/line (4 KiB total)
    Allocated 4096 KiB SRAM memory, in 1 bank at 0x40000000
    Allocated 32 MiB SDRAM memory, in 1 bank at 0x60000000
    Allocated 2048 KiB ROM memory at 0x00000000

    tsim> load zephyr/zephyr.elf
      section: text, addr: 0x40000000, size 16552 bytes
      section: initlevel, addr: 0x400040a8, size 40 bytes
      section: rodata, addr: 0x400040d0, size 484 bytes
      section: datas, addr: 0x400042b4, size 20 bytes
      section: sw_isr_table, addr: 0x400042c8, size 256 bytes
      section: devices, addr: 0x400043c8, size 36 bytes
      Read 436 symbols
    tsim> run
      Initializing and starting from 0x40000000
    *** Booting Zephyr OS build zephyr-v2.4.0-30-ga124c31ec4cf  ***
    Hello World! generic_leon3

References
**********
* `GRLIB IP Library and LEON3, GPL version <https://www.gaisler.com/index.php/downloads/leongrlib>`_
* `TSIM3 LEON3 simulator <https://www.gaisler.com/index.php/products/simulators/tsim3/tsim3-leon3>`_
* `GRMON3 debug monitor <https://www.gaisler.com/index.php/products/debug-tools/grmon3>`_
