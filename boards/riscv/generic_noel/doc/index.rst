.. _generic_noel:

Generic NOEL-V
##############

Overview
********

This board configuration is designed to work with NOEL-V processor
template designs available in the GRLIB GPL distribution.

Hardware
********

The board configuration is compatible most GRLIB NOEL-V FPGA template
designs such as the Digilent Arty A7, Microsemi PolarFire Splash Kit,
Xilinx KCU105 Evaluation kit and Virtex-7 VC707.

Programming and Debugging
*************************

Building
========

Applications for the ``generic_noel`` board configuration can be built as usual
(see :ref:`build_an_application`).
In order to build the application for ``generic_noel``, set the ``BOARD`` variable
to ``generic_noel``.

Running on hardware
===================

Connect with GRMON, then load and run the application. The example below uses
the Digilent Arty A7 board.

.. code-block:: console

    $ grmon -digilent -u
      GRMON debug monitor v3.2.11 eval version

      Copyright (C) 2021 Cobham Gaisler - All rights reserved.
      For latest updates, go to http://www.gaisler.com/
      Comments or bug-reports to support@gaisler.com

      Device ID:           0x291
      GRLIB build version: 4261
      Detected frequency:  40.0 MHz

      Component                            Vendor
      NOEL-V RISC-V Processor              Cobham Gaisler
      GR Ethernet MAC                      Cobham Gaisler
      AHB-to-AHB Bridge                    Cobham Gaisler
      L2-Cache Controller                  Cobham Gaisler
      AHB Debug UART                       Cobham Gaisler
      JTAG Debug Link                      Cobham Gaisler
      EDCL master interface                Cobham Gaisler
      Generic AHB ROM                      Cobham Gaisler
      AHB/APB Bridge                       Cobham Gaisler
      RISC-V CLINT                         Cobham Gaisler
      RISC-V PLIC                          Cobham Gaisler
      Memory controller with EDAC          Cobham Gaisler
      RISC-V Debug Module                  Cobham Gaisler
      AHB/APB Bridge                       Cobham Gaisler
      AHB-to-AHB Bridge                    Cobham Gaisler
      AMBA Trace Buffer                    Cobham Gaisler
      Version and Revision Register        Cobham Gaisler
      AHB Status Register                  Cobham Gaisler
      General Purpose I/O port             Cobham Gaisler
      On chip Logic Analyzer               Cobham Gaisler
      Modular Timer Unit                   Cobham Gaisler
      Generic UART                         Cobham Gaisler

      Use command 'info sys' to print a detailed report of attached cores

    grmon3> load zephyr/zephyr.elf
                 0 vector              16B              [===============>] 100%
                10 exceptions         696B              [===============>] 100%
               2C8 text              11.8kB /  11.8kB   [===============>] 100%
              3224 initlevel           32B              [===============>] 100%
              3244 device_handles      16B              [===============>] 100%
              3254 rodata             712B              [===============>] 100%
              4490 datas               16B              [===============>] 100%
              44A0 sw_isr_table       336B              [===============>] 100%
              45F0 devices             48B              [===============>] 100%
      Total size: 13.67kB (22.39Mbit/s)
      Entry point 0x00000000
      Image zephyr/zephyr.elf loaded

    grmon3> run
    *** Booting Zephyr OS build zephyr-v2.5.0-1786-gaffd3bbe805a  ***
    Hello World! generic_noel


References
**********
* `NOEL-V Processor <https://www.gaisler.com/noelv>`_
* `GRLIB IP Library and NOEL-V, GPL version <https://www.gaisler.com/index.php/downloads/leongrlib>`_
* `GRMON3 debug monitor <https://www.gaisler.com/index.php/products/debug-tools/grmon3>`_
