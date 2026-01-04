.. zephyr:board:: s32k3x4evb

Overview
********

The NXP S32K3X4EVB board is based on the `NXP S32K3 Series`_ family of automotive microcontrollers (MCUs),
which expands the S32K3 series to deliver higher performance, larger memory, and increased vehicle network
communication capability, all with lower power consumption.

The S32K3 MCU features:
- Platform-focused Arm Cortex-M7 cores

Zephyr OS is ported to run on Cortex-M7 core.

- ``s32k3x4evb/s32k344``, for S32K344 Cortex-M7, code executed from code SRAM by default.

Hardware
********

- NXP S32K344
    - Arm Cortex-M7 (2 cores with Lock-Step).
    - 512 kB SRAM for Cortex-M7, with ECC
    - 128 kB D flash with ECC
    - Ethernet switch integrated, CAN FD/XL, QSPI
    - 12-bit ADC, 16-bit eMIOS timer and more.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The Port columns in the Reference Manual (RM) are organized into ports and pins to ensure consistency with
the GPIO driver used in this Zephyr port.

The table below summarizes the mapping between the Port columns in the RM and Zephyr’s ports and pins.
Please consider this mapping when using the GPIO driver or configuring pin multiplexing for device drivers.

+-------------------+--------------------+
|  Ports in S32k344 | Zephyr Ports/Pins  |
+===================+====================+
| GPIO0 - GPIO31    | PA0 - PA31         |
+-------------------+--------------------+
| GPIO32 - GPIO62   | PB0 - PB30         |
+-------------------+--------------------+
| GPIO64 - GPIO95   | PC0 - PC31         |
+-------------------+--------------------+
| GPIO96 - GPIO127  | PD0 - PD31         |
+-------------------+--------------------+
| GPIO128 - GPIO154 | PE0 - PE26         |
+-------------------+--------------------+


LEDs
----

The board has three user RGB LEDs:

=======================  =====  ===============
Devicetree node          Color  Pin
=======================  =====  ===============
led0 / user_led_red      Red    PTA29 : GPIO 29
led1 / user_led_green    Green  PTA30 : GPIO 30
led2 / user_led_blue     Blue   PTA31 : GPIO 31
=======================  =====  ===============

The user can control the LEDs in any way. An output of ``1`` illuminates the LED.

Buttons
-------

The board has two user buttons:

=======================  =======  ==============
Devicetree node          Label    Pin
=======================  =======  ==============
sw0 / user_button_0      USERSW0  PTB26 : GPIO58
sw1 / user_button_1      USERSW1  PTC18 : GPIO82
sw1 / user_button_1      USERSW1  PTB19 : GPIO82
=======================  =======  ==============

UART Debug Interface OpenSDA
-------

The board has UART channel for Debugging logs:

=======================  ================ ==============
Devicetree node          Label            Pin
=======================  ================ ==============
LPUART6_RX               DEBUG_UART0_RX   PTA15 : GPIO15
LPUART6_TX               DEBUG_UART0_TX   PTA16 : GPIO16
=======================  ===============  ==============

Can Phy Interface
-------
Can Connector is available as Jumper J32: Pin 1 CAN_H and Pin 2 CAN_L
CAN2 is connected to the CAN transceiver on the on-board for loopback testing.
=======================  =======    ==============
Devicetree node          Label      Pin
=======================  =======    ==============
CAN0_TX                  CAN0_TX    PTA07 : GPIO7
CAN0_RX                  CAN0_RX    PTA06 : GPIO6
CAN0_EN                  CAN2_TX    PTC21 : GPIO85
CAN0_STB                 CAN0_STB   PTC20 : GPIO84
CAN0_ERRN                CAN0_ERRN  PTC23 : GPIO87
CAN2_RX_PTC14            CAN2_RX    PTC14 : GPIO78
CAN2_RX_PTC15            CAN2_TX    PTC15 : GPIO79
=======================  =======    ==============

System Clock
============

- The Arm Cortex-M7: 160 MHz.

Set-up the Board
================

1) Connect the male micro usb to the board's usb connector connector (``J40``)
and to the host computer via USB.
2) Connect the 12V DC power supply to the board's power connector (``J14``)
3) As board is using OpenSDA for programming and debugging, no additional
hardware is required.
4) Confirm SW1 Main switch is in the ON position (at 1st position).
5) Make sure PEgdbserver driver are installed on the host computer.

Set-up Host pemicrogdbserver Driver
===================================

1) Make Sure OnBoard debugger is in OpenSDA mode and detected by the host computer COM Port as:
   OpenSDA - CDC Serial Port (COMX)` or goto: https://www.pemicro.com/opensda/
   for more details on how to set the board to OpenSDA mode.
2) Make sure PEgdbserver driver are installed on the host computer, If not then GoTo:
   https://www.pemicro.com/products/product_viewDetails.cfm?product_id=15320151&productTab=1000000
   and download the from "PEmicro GDB Server for ARM devices - Eclipse Plugin" link
3) Add the pemicro gdb server path to your environment variables as below:
   (Unzip)DownloadedFolder-->plugins-->(unzip)com.pemicro.debug.gdbjtag.pne.expansion_xxxx-->Win32
4) You can check the board connection using: "pegdbserver_console.exe -showhardware" to valid the connection
   Output Snippet:
   PS C:\Users\user_name>
   P&E GDB Server for Arm(R) devices, Version 9.76.00.00
   Copyright 2018, P&E Microcomputer Systems Inc, All rights reserved
   Loading library C:\PEMicro\PEDrivers\com.pemicro.debug.gdbjtag\win32\gdi\unit_ngs_arm_internal.dll ... Done.
   Command line arguments: -showhardware
   -INTERFACE=USBMULTILINK   -PORT=PEMDFD7A8                ; USB1 : Embedded Multilink Rev A (PEMDFD7A8)[PortNum=21]
   -INTERFACE=USBMULTILINK   -PORT=USB1                     ; USB1 : Embedded Multilink Rev A (PEMDFD7A8)[PortNum=21][DUPLICATE]
5) Now you are ready to build and flash your application to the board.

Programming and Debugging
*************************
.. zephyr:board-supported-runners::
   : nxp_pemicro
Applications for the ``s32k3x4evb`` board can be built in the usual way as
documented in :ref:`build_an_application`.

NOTE: Make sure virtual python environment is activated with west and zephyr installed.

This board supports West runners "nxp_pemicro.py" for the following debug tools:
SERIAL_MONITOR can be used to monitor serial outpur on same COM port as debug interface.
Tested with Windows 11  and Vs code

- :ref:`PEmicro OpenSDA https://www.pemicro.com/opensda/`

You can build and debug the :zephyr:code-sample:`hello_world` sample for the board
``s32k3x4evb/s32k344`` as follows:

Building
=========

"west build -p always -b s32k3x4evb/s32k344 Samples/hello_world -- -DCMAKE_BUILD_TYPE=Debug"

Output Snippet:
"[1/174] Generating include/generated/zephyr/version.h
-- Zephyr version: 4.3.99 (C:/Users/user_name/zephyrproject/zephyr), build: v4.3.0-3078-g2df5c683f274
[174/174] Linking C executable zephyr\zephyr.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:          0 GB         0 GB
             RAM:       72744 B       320 KB     22.20%
            ITCM:          0 GB        64 KB      0.00%
            DTCM:          0 GB       128 KB      0.00%
        IDT_LIST:          0 GB        32 KB      0.00%
Generating files from C:/Users/user_name/zephyrproject/zephyr/build/zephyr/zephyr.elf for board: s32k3x4evb"

Flashing
========

Follow these steps if you just want to download the application to the board and run.

"west flash"

Output Snippet:
"Loading section datas, size 0xf8 lma 0x20410020
Loading section device_states, size 0x10 lma 0x20410118
Loading section log_mpsc_pbuf_area, size 0x38 lma 0x20410128
Loading section log_msg_ptr_area, size 0x4 lma 0x20410160
Loading section k_sem_area, size 0x10 lma 0x20410164
Loading section .last_section, size 0x4 lma 0x20410174
Start address 0x20401fe0, load size 51888
Transfer rate: 52 KB/sec, 836 bytes/write.
No breakpoints currently set.
[Inferior 1 (Remote target) detached]
Disconnected from "127.0.0.1" via 127.0.0.1. Disconnection by port "56083" from 7224"

Debugging
=========
 Run the below Command and use gdb commands to debug the application.

"west debug"

Output Snippet:
"Loading section log_mpsc_pbuf_area, size 0x38 lma 0x20410128
--Type <RET> for more, q to quit, c to continue without paging--
Loading section log_msg_ptr_area, size 0x4 lma 0x20410160
Loading section k_sem_area, size 0x10 lma 0x20410164
Loading section .last_section, size 0x4 lma 0x20410174
Start address 0x20401fe0, load size 51888
Transfer rate: 4 KB/sec, 836 bytes/write.
(gdb)"


References
**********

.. target-notes::
.. _NXP S32K344 172 Evaluation Board details:
   https://www.nxp.com/design/design-center/development-boards-and-designs/S32K3X4EVB-T172
.. _NXP S32K5 Series:
   https://www.nxp.com/products/S32K3
