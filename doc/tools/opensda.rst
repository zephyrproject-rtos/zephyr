:orphan:

.. _nxp_opensda:

NXP OpenSDA
###########

Overview
********

`OpenSDA`_ is a serial and debug adapter that is built into several NXP
evaluation boards. It provides a bridge between your computer (or other USB
host) and the embedded target processor, which can be used for debugging, flash
programming, and serial communication, all over a simple USB cable.

The OpenSDA hardware features a Kinetis K2x microcontroller with an integrated
USB controller. On the software side, it implements a mass storage device
bootloader which offers a quick and easy way to load OpenSDA applications such
as flash programmers, run-control debug interfaces, serial to USB converters,
and more.

Zephyr supports the following debug tools through OpenSDA:

* :ref:`nxp_opensda_pyocd`
* :ref:`nxp_opensda_jlink`

.. _nxp_opensda_firmware:

Program the Firmware
====================

Once you've selected which debug tool you wish to use, you need to program the
associated OpenSDA firmware application to the OpenSDA adapter.

Put the OpenSDA adapter into bootloader mode by holding the reset button while
you power on the board. After you power on the board, release the reset button
and a USB mass storage device called **BOOTLOADER** or **MAINTENANCE** will
enumerate. Copy the OpenSDA firmware application binary to the USB mass storage
device. Power cycle the board, this time without holding the reset button.


.. _nxp_opensda_pyocd:

pyOCD
*****

pyOCD is an Open Source python 2.7 based library for programming and debugging
ARM Cortex-M microcontrollers using CMSIS-DAP.

Host Tools and Firmware
=======================

Follow the instructions in `pyOCD Installation`_ to install the pyOCD flash
tool and GDB server for your host computer.

Select your board in `OpenSDA`_ and download the latest DAPLink firmware
application binary. :ref:`nxp_opensda_firmware` with this application.

Flashing
========

Use the ``make flash`` build target to build your Zephyr application, invoke
the pyOCD flash tool and program your Zephyr application to flash.

  .. code-block:: console

     $ make FLASH_SCRIPT=pyocd.sh flash
     Using /home/maureen/zephyr/boards/arm/frdm_k64f/frdm_k64f_defconfig as base
     Merging /home/maureen/zephyr/tests/include/test.config
     Merging /home/maureen/zephyr/kernel/configs/kernel.config
     Merging prj.conf
     #
     # configuration written to .config
     #
     make[1]: Entering directory '/home/maureen/zephyr'
     make[2]: Entering directory '/home/maureen/zephyr/samples/hello_world/outdir/frdm_k64f'
       GEN     ./Makefile
     scripts/kconfig/conf --silentoldconfig Kconfig
       Using /home/maureen/zephyr as source for kernel
       GEN     ./Makefile
       CHK     include/generated/version.h
       UPD     include/generated/version.h
       DTC     dts/arm/frdm_k64f.dts_compiled
       CHK     include/generated/generated_dts_board.h
       UPD     include/generated/generated_dts_board.h
       CHK     misc/generated/configs.c
       UPD     misc/generated/configs.c
       CHK     include/generated/offsets.h
       UPD     include/generated/offsets.h
       CC      lib/libc/minimal/source/stdlib/strtol.o

     <snip>

       CC      kernel/work_q.o
       AR      kernel/lib.a
       CC      src/main.o
       LD      src/built-in.o
       AR      libzephyr.a
       LINK    zephyr.lnk
       IRQ     isr_tables.c
       CC      isr_tables.o
       LINK    zephyr.elf
       BIN     zephyr.bin
     Flashing frdm_k64f
     Flashing Target Device
     INFO:root:DAP SWD MODE initialised
     INFO:root:K64F not in secure state
     INFO:root:ROM table #0 @ 0xe00ff000 cidr=b105100d pidr=4000bb4c4
     INFO:root:[0]<e000e000:SCS-M4 cidr=b105e00d, pidr=4000bb00c, class=14>
     WARNING:root:Invalid coresight component, cidr=0x0
     INFO:root:[1]<e0001000: cidr=0, pidr=0, component invalid>
     INFO:root:[2]<e0002000:FPB cidr=b105e00d, pidr=4002bb003, class=14>
     WARNING:root:Invalid coresight component, cidr=0x1010101
     INFO:root:[3]<e0000000: cidr=1010101, pidr=101010101010101, component invalid>
     WARNING:root:Invalid coresight component, cidr=0x0
     INFO:root:[4]<e0040000: cidr=0, pidr=0, component invalid>
     INFO:root:[5]<e0041000:ETM-M4 cidr=b105900d, pidr=4000bb925, class=9, devtype=13, devid=0>
     INFO:root:[6]<e0042000:ETB cidr=b105900d, pidr=4003bb907, class=9, devtype=21, devid=0>
     INFO:root:[7]<e0043000:CSTF cidr=b105900d, pidr=4001bb908, class=9, devtype=12, devid=28>
     INFO:root:CPU core is Cortex-M4
     INFO:root:FPU present
     INFO:root:6 hardware breakpoints, 4 literal comparators
     INFO:root:4 hardware watchpoints
     [====================] 100%
     INFO:root:Programmed 12288 bytes (3 pages) at 10.57 kB/s
     make[2]: Leaving directory '/home/maureen/zephyr/samples/hello_world/outdir/frdm_k64f'
     make[1]: Leaving directory '/home/maureen/zephyr'


Debugging
=========

Use the ``make debug`` build target to build your Zephyr application, invoke
the pyOCD GDB server, attach a GDB client, and program your Zephyr application
to flash. It will leave you at a gdb prompt.

  .. code-block:: console

     $ make DEBUG_SCRIPT=pyocd.sh debug
     Using /home/maureen/zephyr/boards/arm/frdm_k64f/frdm_k64f_defconfig as base
     Merging /home/maureen/zephyr/tests/include/test.config
     Merging /home/maureen/zephyr/kernel/configs/kernel.config
     Merging prj.conf
     #
     # configuration written to .config
     #
     make[1]: Entering directory '/home/maureen/zephyr'
     make[2]: Entering directory '/home/maureen/zephyr/samples/hello_world/outdir/frdm_k64f'
       GEN     ./Makefile
     scripts/kconfig/conf --silentoldconfig Kconfig
       Using /home/maureen/zephyr as source for kernel
       GEN     ./Makefile
       CHK     include/generated/version.h
       UPD     include/generated/version.h
       DTC     dts/arm/frdm_k64f.dts_compiled
       CHK     include/generated/generated_dts_board.h
       UPD     include/generated/generated_dts_board.h
       CHK     misc/generated/configs.c
       UPD     misc/generated/configs.c
       CHK     include/generated/offsets.h
       UPD     include/generated/offsets.h
       CC      lib/libc/minimal/source/stdlib/strtol.o

     <snip>

       CC      kernel/work_q.o
       AR      kernel/lib.a
       CC      src/main.o
       LD      src/built-in.o
       AR      libzephyr.a
       LINK    zephyr.lnk
       IRQ     isr_tables.c
       CC      isr_tables.o
       LINK    zephyr.elf
       BIN     zephyr.bin
     pyOCD GDB server running on port 3333
     GNU gdb (GDB) 7.11.0.20160511-git
     Copyright (C) 2016 Free Software Foundation, Inc.
     License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
     This is free software: you are free to change and redistribute it.
     There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
     and "show warranty" for details.
     This GDB was configured as "--host=x86_64-pokysdk-linux --target=arm-zephyr-eabi".
     Type "show configuration" for configuration details.
     For bug reporting instructions, please see:
     <http://www.gnu.org/software/gdb/bugs/>.
     Find the GDB manual and other documentation resources online at:
     <http://www.gnu.org/software/gdb/documentation/>.
     For help, type "help".
     Type "apropos word" to search for commands related to "word"...
     Reading symbols from /home/maureen/zephyr/samples/hello_world/outdir/frdm_k64f/zephyr.elf...done.
     INFO:root:DAP SWD MODE initialised
     INFO:root:K64F not in secure state
     INFO:root:ROM table #0 @ 0xe00ff000 cidr=b105100d pidr=4000bb4c4
     INFO:root:[0]<e000e000:SCS-M4 cidr=b105e00d, pidr=4000bb00c, class=14>
     WARNING:root:Invalid coresight component, cidr=0x0
     INFO:root:[1]<e0001000: cidr=0, pidr=0, component invalid>
     INFO:root:[2]<e0002000:FPB cidr=b105e00d, pidr=4002bb003, class=14>
     WARNING:root:Invalid coresight component, cidr=0x1010101
     INFO:root:[3]<e0000000: cidr=1010101, pidr=101010101010101, component invalid>
     WARNING:root:Invalid coresight component, cidr=0x0
     INFO:root:[4]<e0040000: cidr=0, pidr=0, component invalid>
     INFO:root:[5]<e0041000:ETM-M4 cidr=b105900d, pidr=4000bb925, class=9, devtype=13, devid=0>
     INFO:root:[6]<e0042000:ETB cidr=b105900d, pidr=4003bb907, class=9, devtype=21, devid=0>
     INFO:root:[7]<e0043000:CSTF cidr=b105900d, pidr=4001bb908, class=9, devtype=12, devid=28>
     INFO:root:CPU core is Cortex-M4
     INFO:root:FPU present
     INFO:root:6 hardware breakpoints, 4 literal comparators
     INFO:root:4 hardware watchpoints
     INFO:root:Telnet: server started on port 4444
     INFO:root:GDB server started at port:3333
     Remote debugging using :3333
     INFO:root:One client connected!
     k_cpu_idle () at /home/maureen/zephyr/arch/arm/core/cpu_idle.S:135
     135		bx lr
     Loading section text, size 0x233e lma 0x0
     Loading section devconfig, size 0xa8 lma 0x2340
     Loading section rodata, size 0x5d4 lma 0x23e8
     Loading section datas, size 0x14 lma 0x29bc
     Loading section initlevel, size 0xa8 lma 0x29d0
     [====================] 100%
     INFO:root:Programmed 45056 bytes (3 pages) at 38.21 kB/s
     Start address 0x1b64, load size 10870
     Transfer rate: 9 KB/sec, 1207 bytes/write.
     (gdb)


.. _nxp_opensda_jlink:

Segger J-Link
*************

Segger offers firmware running on the OpenSDA platform which makes OpenSDA
compatible to J-Link Lite, allowing users to take advantage of most J-Link
features like the ultra fast flash download and debugging speed or the
free-to-use GDB Server, by using a low-cost OpenSDA platform for developing on
evaluation boards.

Host Tools and Firmware
=======================

Download and install the `Segger J-Link Software and Documentation Pack`_ to
get the J-Link GDB server for your host computer.

Select your board in `OpenSDA`_ and download the Segger J-Link firmware
application binary. :ref:`nxp_opensda_firmware` with this application.

Flashing
========

The Segger J-Link firmware does not support command line flashing, therefore
the ``make flash`` build target is not supported.

Debugging
=========

Use the ``make debug`` build target to build your Zephyr application, invoke
the J-Link GDB server, attach a GDB client, and program your Zephyr application
to flash. It will leave you at a gdb prompt.

  .. code-block:: console

     $ make DEBUG_SCRIPT=jlink.sh debug
     Using /home/maureen/zephyr/boards/arm/frdm_k64f/frdm_k64f_defconfig as base
     Merging /home/maureen/zephyr/tests/include/test.config
     Merging /home/maureen/zephyr/kernel/configs/kernel.config
     Merging prj.conf
     #
     # configuration written to .config
     #
     make[1]: Entering directory '/home/maureen/zephyr'
     make[2]: Entering directory '/home/maureen/zephyr/samples/hello_world/outdir/frdm_k64f'
       GEN     ./Makefile
     scripts/kconfig/conf --silentoldconfig Kconfig
       Using /home/maureen/zephyr as source for kernel
       GEN     ./Makefile
       CHK     include/generated/version.h
       UPD     include/generated/version.h
       DTC     dts/arm/frdm_k64f.dts_compiled
       CHK     include/generated/generated_dts_board.h
       UPD     include/generated/generated_dts_board.h
       CHK     misc/generated/configs.c
       UPD     misc/generated/configs.c
       CHK     include/generated/offsets.h
       UPD     include/generated/offsets.h
       CC      lib/libc/minimal/source/stdlib/strtol.o

     <snip>

       CC      kernel/work_q.o
       AR      kernel/lib.a
       CC      src/main.o
       LD      src/built-in.o
       AR      libzephyr.a
       LINK    zephyr.lnk
       IRQ     isr_tables.c
       CC      isr_tables.o
       LINK    zephyr.elf
       BIN     zephyr.bin
     JLink GDB server running on port 2331
     SEGGER J-Link GDB Server V6.14b Command Line Version

     JLinkARM.dll V6.14b (DLL compiled Mar  9 2017 08:48:20)

     -----GDB Server start settings-----
     GDBInit file:                  none
     GDB Server Listening port:     2331
     SWO raw output listening port: 2332
     Terminal I/O port:             2333
     Accept remote connection:      yes
     Generate logfile:              off
     Verify download:               off
     Init regs on start:            off
     Silent mode:                   off
     Single run mode:               on
     Target connection timeout:     0 ms
     ------J-Link related settings------
     J-Link Host interface:         USB
     J-Link script:                 none
     J-Link settings file:          none
     ------Target related settings------
     Target device:                 MK64FN1M0xxx12
     Target interface:              SWD
     Target interface speed:        1000kHz
     Target endian:                 little

     Connecting to J-Link...
     GNU gdb (GDB) 7.11.0.20160511-git
     Copyright (C) 2016 Free Software Foundation, Inc.
     License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
     This is free software: you are free to change and redistribute it.
     There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
     and "show warranty" for details.
     This GDB was configured as "--host=x86_64-pokysdk-linux --target=arm-zephyr-eabi".
     Type "show configuration" for configuration details.
     For bug reporting instructions, please see:
     <http://www.gnu.org/software/gdb/bugs/>.
     Find the GDB manual and other documentation resources online at:
     <http://www.gnu.org/software/gdb/documentation/>.
     For help, type "help".
     Type "apropos word" to search for commands related to "word"...
     Reading symbols from /home/maureen/zephyr/samples/hello_world/outdir/frdm_k64f/zephyr.elf...done.
     J-Link is connected.
     Firmware: J-Link OpenSDA 2 compiled Feb 28 2017 19:27:57
     Hardware: V1.00
     S/N: 621000000
     Checking target voltage...
     Target voltage: 3.30 V
     Listening on TCP/IP port 2331
     Connecting to target...Connected to target
     Waiting for GDB connection...Remote debugging using :2331
     Connected to 127.0.0.1
     Reading all registers
     Read 4 bytes @ address 0x00001A04 (Data = 0xBF004770)
     Read 2 bytes @ address 0x000019FC (Data = 0x4040)
     Read 2 bytes @ address 0x000019FE (Data = 0xF380)
     Read 2 bytes @ address 0x00001A00 (Data = 0x8811)
     Read 2 bytes @ address 0x00001A02 (Data = 0xBF30)
     k_cpu_idle () at /home/maureen/zephyr/arch/arm/core/cpu_idle.S:135
     135		bx lr
     Halting target CPU...
     ...Target halted (PC = 0x00001A04)
     Loading section text, size 0x233e lma 0x0
     Downloading 4096 bytes @ address 0x00000000
     Downloading 4096 bytes @ address 0x00001000
     Downloading 830 bytes @ address 0x00002000
     Loading section devconfig, size 0xa8 lma 0x2340
     Downloading 168 bytes @ address 0x00002340
     Loading section rodata, size 0x5d4 lma 0x23e8
     Downloading 1492 bytes @ address 0x000023E8
     Loading section datas, size 0x14 lma 0x29bc
     Downloading 20 bytes @ address 0x000029BC
     Loading section initlevel, size 0xa8 lma 0x29d0
     Downloading 168 bytes @ address 0x000029D0
     Start address 0x1b64, load size 10870
     Writing register (PC = 0x641b0000)
     Transfer rate: 5307 KB/sec, 1552 bytes/write.
     Read 4 bytes @ address 0x00001B64 (Data = 0xF3802010)
     Resetting target
     Resetting target
     (gdb)


Console
=======

If you configured your Zephyr application to use a UART console (most boards
enable this by default), open a serial terminal (minicom, putty, etc.) with the
following settings:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

If you configured your Zephyr application to use `Segger RTT`_ console instead,
open telnet:

  .. code-block:: console

     $ telnet localhost 19021
     Trying 127.0.0.1...
     Connected to localhost.
     Escape character is '^]'.
     SEGGER J-Link V6.14b - Real time terminal output
     J-Link OpenSDA 2 compiled Feb 28 2017 19:27:57 V1.0, SN=621000000
     Process: JLinkGDBServer


.. _OpenSDA:
   http://www.nxp.com/opensda

.. _Segger J-Link OpenSDA:
   https://www.segger.com/opensda.html

.. _Segger J-Link Software and Documentation Pack:
   https://www.segger.com/downloads/jlink

.. _Segger RTT:
    https://www.segger.com/jlink-rtt.html

.. _pyOCD Installation:
   https://github.com/mbedmicro/pyOCD#installation
