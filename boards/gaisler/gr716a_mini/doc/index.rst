.. zephyr:board:: gr716a_mini

Overview
********

The GR716-MINI development board provides:

* GR716 microcontroller
* SPI Flash PROM, 32 MiB
* SRAM, 2 MiB
* FTDI USB interface for UART debug link (AHBUART) and application UART
  (APBUART).
* 4x MMCX connectors (2 ADC, 2 DAC)
* Miniature 80 pin mezzanine connector (bottom side)

Hardware
********

Console Output
==============

By default, the kernel is configured to send console output to the
first APBUART peripheral (apbuart0). The UART debug forwarding setting,
if enabled in GRMON, is preserved.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Applications for the ``gr716a_mini`` board configuration can be built
as usual (see :ref:`build_an_application`).
In order to build the application for ``gr716a_mini``, set the ``BOARD``
variable to ``gr716a_mini``.

The application is linked to the on-chip tightly coupled memory by
default.

Running on hardware
===================

Connect with GRMON, then load and run the application.

.. code-block:: console

    $ grmon -u -cginit 0x00010000 -uart /dev/ttyUSB0
      GRMON debug monitor v3.2.8

      Copyright (C) 2020 Cobham Gaisler - All rights reserved.
      For latest updates, go to http://www.gaisler.com/
      Comments or bug-reports to support@gaisler.com

      Device ID:           0x716
      GRLIB build version: 4204
      Detected system:     GR716
      Detected frequency:  20.0 MHz

      Component                            Vendor
      AHB-to-AHB Bridge                    Cobham Gaisler
      MIL-STD-1553B Interface              Cobham Gaisler
      GRSPW2 SpaceWire Serial Link         Cobham Gaisler
      SPI to AHB Bridge                    Cobham Gaisler
      I2C to AHB Bridge                    Cobham Gaisler
      CAN Controller with DMA              Cobham Gaisler
      CAN Controller with DMA              Cobham Gaisler
      AHB Debug UART                       Cobham Gaisler
      AHB-to-AHB Bridge                    Cobham Gaisler
      PacketWire Receiver with DMA         Cobham Gaisler
      PacketWire Transmitter with DMA      Cobham Gaisler
      GRDMAC DMA Controller                Cobham Gaisler
      GRDMAC DMA Controller                Cobham Gaisler
      GRDMAC DMA Controller                Cobham Gaisler
      GRDMAC DMA Controller                Cobham Gaisler
      Dual-port SPI Slave                  Cobham Gaisler
      LEON3FT SPARC V8 Processor           Cobham Gaisler
      AHB-to-AHB Bridge                    Cobham Gaisler
      AHB Memory Scrubber                  Cobham Gaisler
      AHB-to-AHB Bridge                    Cobham Gaisler
      AHB Debug UART                       Cobham Gaisler
      Dual-port AHB(/CPU) On-Chip RAM      Cobham Gaisler
      Dual-port AHB(/CPU) On-Chip RAM      Cobham Gaisler
      Generic AHB ROM                      Cobham Gaisler
      Memory controller with EDAC          Cobham Gaisler
      SPI Memory Controller                Cobham Gaisler
      SPI Memory Controller                Cobham Gaisler
      AHB/APB Bridge                       Cobham Gaisler
      AHB/APB Bridge                       Cobham Gaisler
      AHB/APB Bridge                       Cobham Gaisler
      AHB/APB Bridge                       Cobham Gaisler
      Memory controller with EDAC          Cobham Gaisler
      LEON3 Debug Support Unit             Cobham Gaisler
      AHB/APB Bridge                       Cobham Gaisler
      AMBA Trace Buffer                    Cobham Gaisler
      Multi-processor Interrupt Ctrl.      Cobham Gaisler
      Modular Timer Unit                   Cobham Gaisler
      Modular Timer Unit                   Cobham Gaisler
      GR716 AMBA Protection unit           Cobham Gaisler
      Clock gating unit                    Cobham Gaisler
      Clock gating unit                    Cobham Gaisler
      General Purpose Register             Cobham Gaisler
      LEON3 Statistics Unit                Cobham Gaisler
      AHB Status Register                  Cobham Gaisler
      CCSDS TDP / SpaceWire I/F            Cobham Gaisler
      General Purpose Register Bank        Cobham Gaisler
      General Purpose Register             Cobham Gaisler
      GR716 AMBA Protection unit           Cobham Gaisler
      GR716 Bandgap                        Cobham Gaisler
      GR716 Brownout detector              Cobham Gaisler
      GR716 Phase-locked loop              Cobham Gaisler
      Generic UART                         Cobham Gaisler
      Generic UART                         Cobham Gaisler
      Generic UART                         Cobham Gaisler
      Generic UART                         Cobham Gaisler
      Generic UART                         Cobham Gaisler
      Generic UART                         Cobham Gaisler
      AHB Status Register                  Cobham Gaisler
      ADC / DAC Interface                  Cobham Gaisler
      SPI Controller                       Cobham Gaisler
      SPI Controller                       Cobham Gaisler
      PWM generator                        Cobham Gaisler
      General Purpose I/O port             Cobham Gaisler
      General Purpose I/O port             Cobham Gaisler
      AMBA Wrapper for OC I2C-master       Cobham Gaisler
      AMBA Wrapper for OC I2C-master       Cobham Gaisler
      GR716 Analog-to-Digital Conv         Cobham Gaisler
      GR716 Analog-to-Digital Conv         Cobham Gaisler
      GR716 Analog-to-Digital Conv         Cobham Gaisler
      GR716 Analog-to-Digital Conv         Cobham Gaisler
      GR716 Analog-to-Digital Conv         Cobham Gaisler
      GR716 Analog-to-Digital Conv         Cobham Gaisler
      GR716 Analog-to-Digital Conv         Cobham Gaisler
      GR716 Analog-to-Digital Conv         Cobham Gaisler
      GR716 Digital-to-Analog Conv         Cobham Gaisler
      GR716 Digital-to-Analog Conv         Cobham Gaisler
      GR716 Digital-to-Analog Conv         Cobham Gaisler
      GR716 Digital-to-Analog Conv         Cobham Gaisler
      I2C Slave                            Cobham Gaisler
      I2C Slave                            Cobham Gaisler
      PWM generator                        Cobham Gaisler
      LEON3 Statistics Unit                Cobham Gaisler
      General Purpose Register             Cobham Gaisler

      Use command 'info sys' to print a detailed report of attached cores

    grmon3> load zephyr/zephyr.elf
          31000000 text              16.2kB /  16.2kB   [===============>] 100%
          300040A8 initlevel           40B              [===============>] 100%
          300040D0 rodata             484B              [===============>] 100%
          300042B4 datas               20B              [===============>] 100%
          300042C8 sw_isr_table       256B              [===============>] 100%
          300043C8 devices             36B              [===============>] 100%
      Total size: 16.98kB (1.91Mbit/s)
      Entry point 0x31000000
      Image zephyr/zephyr.elf loaded

    grmon3> run
    *** Booting Zephyr OS build zephyr-v2.4.0-788-gc82a8736a65e  ***
    Hello World! gr716a_mini


Running in simulation
=====================

The same application binary can be simulated with the TSIM3 LEON3 simulator.

.. code-block:: console

    $ tsim-leon3 -freq 20 -gr716

     TSIM3 LEON3 SPARC simulator, version v3.0.2

     Copyright (C) 2020, Cobham Gaisler - all rights reserved.
     For latest updates, go to https://www.gaisler.com/
     Comments or bug-reports to support@gaisler.com

    Number of CPUs: 1
    register windows: 31
    system frequency: 20.000 MHz
    using 64-bit time
    Allocated 128 KiB local instruction RAM memory at 0x31000000
    Allocated 64 KiB local data RAM memory at 0x30000000
    Allocated 4096 KiB SRAM memory, in 1 bank at 0x40000000
    Allocated 2048 KiB ROM memory at 0x01000000
    Allocated 16384 KiB SPIM ROM memory at 0x02000000
    Allocated 16384 KiB SPIM ROM memory at 0x04000000

    tsim> load zephyr/zephyr.elf
      section: text, addr: 0x31000000, size 16956 bytes
      section: initlevel, addr: 0x30000000, size 40 bytes
      section: rodata, addr: 0x30000028, size 484 bytes
      section: datas, addr: 0x3000020c, size 20 bytes
      section: sw_isr_table, addr: 0x30000220, size 256 bytes
      section: devices, addr: 0x30000320, size 36 bytes
      Read 438 symbols
    tsim> run
      Initializing and starting from 0x31000000
    *** Booting Zephyr OS build zephyr-v2.4.0-788-gc82a8736a65e  ***
    Hello World! gr716a_mini


References
**********
* `GR716 Evaluation and Development Boards <https://www.gaisler.com/index.php/products/boards/gr716-boards>`_
* `TSIM3 LEON3 simulator <https://www.gaisler.com/index.php/products/simulators/tsim3/tsim3-leon3>`_
* `GRMON3 debug monitor <https://www.gaisler.com/index.php/products/debug-tools/grmon3>`_
