.. _frdm_k64f:

Board Configuration: frdm_k64f
#####################################

Overview
********

The frdm_k64f board configuration is used by Zephyr applications
that run on the Freescale Freedom Development Platform (FRDM-K64F).
It provides support for an ARM Cortex-M4 CPU and the following devices:

* Nested Vectored Interrupt Controller (NVIC)

* System Tick System Clock (SYSTICK)

* Serial Port over USB (K20)

See `Procedures`_ for using third-party tools to load
and debug (in system mode with no OS awareness) a
Zephyr application image on the target. Debugging is
done with GNU Debugger (GDB), using Eclipse plugins.

.. note::
   This board configuration may work with similar boards,
   but they are not officially supported.

Supported Boards
****************

The frdm_k64f board configuration has been tested to run on the
Freescale Freedom Development Platform.  The physical characteristics of
this board (including pin names, jumper settings, memory mappings, ...)
can be found below.  No claims are made about its suitability for use with
any other hardware system.

Pin Names
=========

**LED (RGB)**

* LED_RED = PTB22
* LED_GREEN = PTE26
* LED_BLUE = PTB21
* Original LED Naming

  * LED1 = LED_RED
  * LED2 = LED_GREEN
  * LED3 = LED_BLUE
  * LED4 = LED_RED

**Push buttons**

* SW2 = PTC6
* SW3 = PTA4

**USB Pins**

* USBTX = PTB17
* USBRX = PTB16

**Arduino Headers**

* D0 = PTC16
* D1 = PTC17
* D2 = PTB9
* D3 = PTA1
* D4 = PTB23
* D5 = PTA2
* D6 = PTC2
* D7 = PTC3
* D8 = PTA0
* D9 = PTC4
* D10 = PTD0
* D11 = PTD2
* D12 = PTD3
* D13 = PTD1
* D14 = PTE25
* D15 = PTE24
* A0 = PTB2
* A1 = PTB3
* A2 = PTB10
* A3 = PTB11
* A4 = PTC10
* A5 = PTC11

**I2C pins**

* I2C_SCL = D15
* I2C_SDA = D14
* DAC0_OUT = 0xFEFE /\* DAC does not have a Pin Name in RM \*/

Jumpers & Switches
==================

The Zephyr kernel uses the FRDM-K64F default switch and jumper settings.

The default switch settings for the Freescale FRDM-K64F are:

+---------------+------------+---------------+
| Switch Number | Switch     | ON Switch OFF |
+===============+============+===============+
|  J14          |            |      x        |
+---------------+------------+---------------+
|  J21          |     x      |               |
+---------------+------------+---------------+
|  J25          | SDA + SW1  |      MCU      |
+---------------+------------+---------------+

Memory Mappings
===============

The frdm_k64f board configuration uses the
following default hardware memory map addresses and sizes:

+--------------------------+---------+------------------+
| Physical Address         | Size    | Access To        |
+==========================+=========+==================+
| 0xFFFFFFFF - 0xE0100000  |    -    | System           |
+--------------------------+---------+------------------+
| 0xE0100000 - 0xE0040000  |    -    | External Private |
|                          |         | Peripheral Bus   |
+--------------------------+---------+------------------+
| 0xE0100000 - 0xE00FF000  |    -    | ROM Table        |
+--------------------------+---------+------------------+
| 0xE00FF000 - 0xE0042000  |    -    | External PPB     |
+--------------------------+---------+------------------+
| 0xE0042000 - 0xE0041000  |    -    | ETM              |
+--------------------------+---------+------------------+
| 0xE0041000 - 0xE0040000  |    -    | TIPU             |
+--------------------------+---------+------------------+
| 0xE0040000 - 0xE0000000  |    -    | Internal Private |
|                          |         | Peripheral Bus   |
+--------------------------+---------+------------------+
| 0xE0040000 - 0xE000F000  |    -    | Reserved         |
+--------------------------+---------+------------------+
| 0xE000F000 - 0xE000E000  |    -    | SCS              |
+--------------------------+---------+------------------+
| 0xE000E000 - 0xE0003000  |    -    | Reserved         |
+--------------------------+---------+------------------+
| 0xE0003000 - 0xE0002000  |    -    | FPB              |
+--------------------------+---------+------------------+
| 0xE0002000 - 0xE0001000  |    -    | DWT              |
+--------------------------+---------+------------------+
| 0xE0001000 - 0xE0000000  |    -    | ITM              |
+--------------------------+---------+------------------+
| 0xE0000000 - 0xA0000000  |   1GB   | External device  |
+--------------------------+---------+------------------+
| 0xA0000000 - 0x60000000  |   1GB   | External RAM     |
+--------------------------+---------+------------------+
| 0x60000000 - 0x40000000  |  .5GB   | Peripheral       |
+--------------------------+---------+------------------+
| 0x44000000 - 0x42000000  |  32MB   | Bit band alias   |
+--------------------------+---------+------------------+
| 0x42000000 - 0x40100000  |  31MB   | unnamed          |
+--------------------------+---------+------------------+
| 0x40100000 - 0x40000000  |   1MB   | Bit band region  |
+--------------------------+---------+------------------+
| 0x40000000 - 0x20000000  |  .5GB   | SRAM             |
+--------------------------+---------+------------------+
| 0x24000000 - 0x22000000  |  32MB   | Bitband alias    |
+--------------------------+---------+------------------+
| 0x22000000 - 0x20100000  |  31MB   | unnamed          |
+--------------------------+---------+------------------+
| 0x20100000 - 0x20000000  |   1MB   | Bitband region   |
+--------------------------+---------+------------------+
| 0x20000000 - 0x00000000  |  .5GB   | Code             |
+--------------------------+---------+------------------+

For a diagram, see  `Cortex-M3 Revision r2p1 Technical Reference Manual page 3-11`_.

.. _Cortex-M3 Revision r2p1 Technical Reference Manual page 3-11: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0337h/index.

Component Layout
================

Refer to page 2 of the FRDM-K64F Freedom Module User's Guide,
Rev. 0, 04/2014 (Freescale FRDMK64FUG) for a component layout
block diagram. See
http://infocenter.arm.com/help/topic/com.arm.doc.dui0552a/DUI0552A_cortex_m3_dgug.pdf

Supported Features
******************

The frdm_k64f board configuration supports the following
hardware features:

+--------------+------------+----------------------+
| Interface    | Controller | Driver/Component     |
+==============+============+======================+
| NVIC         | on-chip    | nested vectored      |
|              |            | interrupt controller |
+--------------+------------+----------------------+
| SYSTICK      | on-chip    | system clock         |
+--------------+------------+----------------------+
| UART  1      | on-chip    | serial port          |
| (OpenSDA v2) |            |                      |
+--------------+------------+----------------------+


Other hardware features are not currently supported by the Zephyr kernel.
See `vendor documentation`_ for a complete list of
Freescale FRDM-K64F board hardware features.

.. _vendor documentation: http://infocenter.arm.com/help/topic/com.arm.doc.dui0552a/DUI0552A_cortex_m3_dgug.pdf


Interrupt Controller
====================

There are 15 fixed exceptions including exceptions 12 (debug
monitor) and 15 (SYSTICK) that behave more as interrupts
than exceptions. In addition, there can be a variable number
of IRQs. Exceptions 7-10 and 13 are reserved. They don't need
handlers.

A Cortex-M3/4-based board uses vectored exceptions. This
means each exception calls a handler directly from the
vector table.

Handlers are provided for exceptions 1-6, 11-12, and 14-15.
The table here identifies the handlers used for each exception.

+------+------------+----------------+--------------------------+
| Exc# | Name       | Remarks        | Used by Zephyr Kernel    |
+======+============+================+==========================+
| 1    | Reset      |                | system initialization    |
+------+------------+----------------+--------------------------+
| 2    | NMI        |                | system fatal error       |
+------+------------+----------------+--------------------------+
| 3    | Hard fault |                | system fatal error       |
+------+------------+----------------+--------------------------+
| 4    | MemManage  | MPU fault      | system fatal error       |
+------+------------+----------------+--------------------------+
| 5    | Bus        |                | system fatal error       |
+------+------------+----------------+--------------------------+
| 6    | Usage      | undefined      | system fatal error       |
|      | fault      | instruction,   |                          |
|      |            | or switch      |                          |
|      |            | attempt to ARM |                          |
|      |            | mode           |                          |
+------+------------+----------------+--------------------------+
| 11   | SVC        |                | context switch           |
+------+------------+----------------+--------------------------+
| 12   | Debug      |                | system fatal error       |
|      | monitor    |                |                          |
+------+------------+----------------+--------------------------+
| 14   | PendSV     |                | context switch           |
+------+------------+----------------+--------------------------+
| 15   | SYSTICK    |                | system clock             |
+------+------------+----------------+--------------------------+

.. note::
   After a reset, all exceptions have a priority of 0.
   Interrupts cannot run at priority 0 for the interrupt
   locking mechanism and exception handling to function properly.

Interrupts
----------

Interrupt numbers are virtual and numbered from 0 through N,
regardless of how the interrupt controllers are set up.
However, with the Cortex-M3 which has only one NVIC, interrupts map
directly to physical interrupts 0 through N, and to exceptions
16 through (N + 16).

The Cortex-M4 has an 8-bit priority register. However, some of the
lowest-significant bits are often not implemented. When citing
priorities, a priority of 1 means the first priority lower than 0,
not necessarily the priority whose numerical value is 1.
For example, when only the top three bits are implemented,
priority 1 has a priority numerical value of 0x20h.

When specifying an interrupt priority either to connect
an ISR or to set the priority of an interrupt, use low numbers.
For example, if 3 bits are implemented, use 1, 2, and 3,
not 0x20h, 0x40h, and 0x60h.

Interrupt priority is set using the *prio* parameter of
:c:macro:`irq_connect()` or :c:func:`irq_connect_dynamic()`.

The range of available priorities is different if using Zero Latency Interrupts
(ZLI) or not.

When not using ZLI:

* 2 to 2\ :sup:`n`\ -2, where *n* is the number of implemented bits
  (e.g. 2 to 14 for 4 implemented bits)

* Interrupt locking is done by setting :envvar:`BASEPRI` to 2, setting
  exceptions 4, 5, 6, and 11 to priority 1, and setting all other exceptions,
  including interrupts, to a lower priority (2+).

When using ZLI:

* 3 to 2\ :sup:`n`\ -2, where *n* is the number of implemented bits
  (e.g. 3 to 6 for 3 implemented bits)

* Interrupt locking is done by setting :envvar:`BASEPRI` to 3, setting
  exceptions 4, 5, 6, and 11 to priority 1, setting ZLI interupts to priority 2
  and setting all other exceptions, including interrupts, to a lower priority
  (3+).

.. note::
   The hard fault exception is always kept at priority 0 so that it is
   allowed to occur while handling another exception.

.. note::
   The PendSV exception is always installed at the lowest priority
   available, and that priority level is thus not avaialble to other
   exceptions and interrupts.

Interrupt Tables
----------------

There are a number of ways of setting up the interrupt
table depending on the range of flexibility and performance
needed. The two following kconfig options drive the interrupt
table options:

:option:`SW_ISR_TABLE` and :option:`SW_ISR_TABLE_DYNAMIC`

Depending on whether static tables are provided by the platform
configuration or by the application, two other kconfig options
are available:

:option:`SW_ISR_TABLE_STATIC_CUSTOM` and
:option:`IRQ_VECTOR_TABLE_CUSTOM`

The following interrupt table scenarios exist:

:option:`SW_ISR_TABLE=y`, :option:`SW_ISR_TABLE_DYNAMIC=y`
   For maximum ease of use, maximum flexibility, a larger
   footprint, and weaker performance.

   This is the default setup. The vector table is static
   and uses the same handler for all entries. The handler
   finds out at runtime what interrupt is running and
   invokes the correct ISR. An argument is passed to the
   ISR when the ISR is connected.

   The table, in the data section and therefore in SRAM,
   has one entry per interrupt request (IRQ) in the vector
   table. An entry in that table consists of two words, one
   for the ISR and one for the argument. The table size,
   calculated by multiplying the number of interrupts by 8
   bytes, can add significant overhead.

   In this scenario, some demuxing must take place which
   causes a delay before the ISR runs. On the plus side,
   the vector table can be automatically generated by the Zephyr kernel.
   Also, an argument can be passed to the ISR, allowing
   multiple devices of the same type to share the same ISR.
   Sharing an ISR can potentially save as much, or even more,
   memory than a software table implementation might save.

   Another plus is that the vector table is able to take
   care of the exception handling epilogue because the
   handler is installed directly in the vector table.

:option:`SW_ISR_TABLE=y`, :option:`SW_ISR_TABLE_DYNAMIC=n`
   For advanced use, medium flexibility, a medium footprint,
   and medium performance.

   In this setup, the software table exists, but it is static
   and pre-populated. ISRs can have arguments with an automatic
   exception handling epilogue. Table pre-population provides
   better boot performance because there is no call to
   :c:func:`irq_connect` during boot up; however,
   the user must provide a file to override the platform's
   default ISR table defined in :file:`sw_isr_table.S`.
   This file must contain the :makevar:`_sw_isr_table[]`
   variable initialized with each interrupt's ISR. The variable
   is an array of type struct _IsrTableEntry. When a user
   provides their own :file:`sw_isr_table.c`, the type can be found
   by including :file:`sw_isr_table.h`.

:option:`SW_ISR_TABLE=n`
   For advanced use, no flexibility, the best footprint, and
   the best performance.

   In this setup, there is no software table. ISRs are installed
   directly in the vector table using the **_irq_vector_table** symbol
   in the .irq_vector_table section. The symbol resolves to an
   array of words containing the addresses of ISRs. The linker
   script puts that section directly
   after the section containing the first 16 exception vectors
   (.exc_vector_table) to form the full vector table in ROM.
   An example of this can be found in the platform's
   :file:`irq_vector_table.c`.  Because ISRs
   hook directly into the vector table, this setup gives the best
   possible performance regarding latency when handling interrupts.

   When the ISR is hooked directly to the vector, the ISR
   must manually invoke the :c:func:`_IntExit()` function
   as its very last action.

.. note::
   This configuration prevents the use of tickless idle.

:option:`SW_ISR_TABLE=y`, :option:`SW_ISR_TABLE_STATIC_CUSTOM=y`
   For overriding the static ISR tables defined by the platform:

   In this setup, the platform provides the **_irq_vector_table**
   symbol and data in :file:`sw_isr_table.s`.

:option:`SW_ISR_TABLE=n`, :option:`IRQ_VECTOR_TABLE_CUSTOM=y`
   In this setup, the platform provides the **_irq_vector_table** symbol
   and data in `irq_vector_table.c`.

Configuration Options
=====================

:option:`LDREX_STREX_AVAILABLE`
      Set to 'n' when the ldrex/strex instructions are not available.

:option:`DATA_ENDIANNESS_LITTLE`
      Set to 'n' when the data sections are big endian.

:option:`STACK_ALIGN_DOUBLE_WORD`
      Set to 'n' only when there is a good reason to do it.

:option:`NUM_IRQ_PRIO_BITS`
      The board configuration sets this to the correct value for the board
      ("4" for FRDM board, IIRC).

:option:`RUNTIME_NMI`
      The kernel provides a simple NMI handler that simply
      hangs in a tight loop if triggered. This fills the
      requirement that there must be an NMI handler installed
      when the CPU boots.If a custom handler is needed,
      enable this option and attach it via _NmiHandlerSet().

:option:`NUM_IRQS`
      The board configuration sets this value to the correct number of
      interrupts available on the board. The default is '34'.

:option:`SW_ISR_TABLE`
      Set to 'n' when the board configuration does not provide one.

:option:`SW_ISR_TABLE_DYNAMIC`
      Set to 'n' to override the default.

System Clock
============
FRDM-K64F uses an external oscillator/resonator.
It can have a frequency range of 32.768 KHz to 50 MHz.

Serial Port
===========

The FRDM_K64F board has a single out-of-the-box available
serial communication channel that uses the CPU's UART0.
It is connected via a "USB Virtual Serial Port"
over the OpenSDA USB connection.

See the `Procedures`_ in the next section for instruction
on how to direct output from the board to a console.

Procedures
**********

Use the following procedures:

* `Loading a Project Image with FRDM K64F firmware`_

* `Installing Hardware Debug Support on the Host and Target`_

* `Installing the IDE and Eclipse Plug-ins`_

* `Configuring the J-Link Debugger`_

* `Programming Flash with J-link`_

Loading a Project Image with FRDM K64F Firmware
===============================================

Load a project image with FRDM K64F firmware from the mbed project
if you only need to load and run an image without debug tools.
FRDM K64F firmware is available for the board (and may already be
pre-installed).


Prerequisite
------------

Although FRDM K64F firmware may be pre-installed on the
FRDM_K64F, you must replace it with the latest version.

Steps
-----

1. Go to the `FRDM K64F firmware instructions
   <http://developer.mbed.org/handbook/Firmware-FRDM-K64F>`_.

2. Download the lastest version of the FRDM K64F firmware from the mbed project.

3. Update the FRDM K64F firmware using the following `online
   instructions <http://developer.mbed.org/handbook/Firmware-FRDM-K64F>`_:

    a) *Enter Bootloader mode*.
    b) *Update Using Windows and Linux*.
    c) *Power Down, Power Up*.

3. Follow the online instructions to `Connect the microcontroller to a PC
   <https://developer.mbed.org/platforms/frdm-k64f/#pc-configuration>`_.

    a) *Connect your microcontroller to a PC*.
    b) *Click the MBED.HTM link to log in*.

4. Follow the online instructions to `Configure a terminal application
   <http://mbed.org/handbook/Terminals>`_.

    a) *Install a Terminal Application*.
    b) *Setup the Connection Use COMx at 8-N-1 with 115200 baud*.

   The Status light on the FRDM K64F Microcontroller flickers
   when you type in the terminal application.

5. Configure the host to run a progam binary using the online instructions
   `Downloading a Program
   <http://mbed.org/platforms/frdm-k64f/#pc-configuration>`_.

    a) *Save a program binary (.bin) to the FRDM Platform*.
    b) *Press the Reset button*.
    c) *Download a program binary*.

6. Disconnect and re-connect the terminal serial port
   connection after copying each :file:`.bin` file.

Installing Hardware Debug Support on the Host and Target
========================================================

.. Caution::
   Debug firmware and FRDM K64F firmware cannot be used together.
   Debug firmware overwrites FRDM K64F firmware when installed.


Install hardware debug support on the host and target to use debug tools.

Prerequisites
-------------

* You understand that Segger does not warranty or support OpenSDA V2 firmware.

* You comply with all OpenSDA V2 firmware conditions of use, but particularly:

    - Use with Freescale target devices only. Use with other devices
          is prohibited and illegal.

    - Use with evaluation boards only; not with custom hardware.

    - Use for development and/or evaluation purposes only.

* You have licensed J-Link firmware.

* You have USB drivers for J-Links with VCOM support.


Steps
-----

1.  Go to the `J-Link
    <https://www.segger.com/jlink-software.html>`_ site.

2.  Locate the section, **J-Link software &
    documentation pack for Linux ARM systems** and
    click the **Download** button for **Software and
    documentation pack for Linux ARM systems V5.00b**.

3.  Go to `Segger OpenSDA <https://www.segger.com/opensda.html>`_.

4.  Download :file:`JLink_OpenSDA_V2_2015-04-23.zip`.

5.  Install the :program:`USB Driver for J-Link with Virtual COM
    Port` on the PC.

6.  Extract the OpenSDA image from the download.

7.  Press and hold the board **Reset** button while
    connecting the board to the PC with a USB cable.

    The OpenSDA platform starts in MSD mode.

8.  From the PC, drag & drop the :file:`.sda/.bin` file to
    the board to load the firmware.

9.  Disconnect and reconnect the board.

    The OpenSDA platform is now available on the PC as a
    J-Link appearance.

10. Run the :program:`J-Link Commander` (JLinkExe on Linux)
    program on the PC to test if the J-Link connects
    to the target.

Installing the IDE and Eclipse Plug-ins
=======================================

Install the GNU ARM Eclipse plug-in to debug with J-Link
in an Eclipse environment.


Prerequisites
-------------

* You already have the GDB Server and J-Link
  Commander utility you downloaded with the
  `Software and documentation pack for Linux ARM systems V5
  <https://www.segger.com/jlink-software.html>`_.

* Review the `GNU Tools for ARM Embedded Processors
  <https://launchpad.net/gcc-arm-embedded>`_ documentation.


Steps
-----

1.  Download and install a Linux version of `Eclipse IDE for
    C/C++ Developers
    <https://www.eclipse.org/downloads/packages/eclipse-ide-cc-developers/lunasr2>`_
    if you do not have Eclipse installed already.

2.  Download and install the
    `GNU ARM Eclipse Plug-ins <http://sourceforge.net/projects/gnuarmeclipse/>`_,
    and follow the `online instructions
    <http://gnuarmeclipse.livius.net/blog/>`_.

3.  Follow the online instructions to install the
    `GDB Server <https://www.segger.com/jlink-gdb-server.html>`_.

4.  Download and install the
    `GCC, the GNU Compiler Collection <https://gcc.gnu.org/>`_.
    [This step does not apply to Wind River customers.]

5.  Download and install `GDB: The GNU Project Debugger
    <http://www.gnu.org/software/gdb/download/>`_.
    [This step does not apply to Wind River customers.]

6.  Download and install the `J-Link hardware debugging
    Eclipse plug-in <http://gnuarmeclipse.livius.net/blog/jlink-debugging/>`_.



Configuring the J-Link Debugger
===============================

Configure the J-Link Debugger to work with all the software installed.


Prerequisites
-------------

* The `J-Link hardware debugging Eclipse plug-in
  <http://gnuarmeclipse.livius.net/blog/jlink-debugging/>`_ page is open.


Steps
-----

1.  Follow the online configuration instructions that
    should be open already from the previous procedure,
    then optimize the configuration using the remaining
    steps in this procedure.

2.  Create an empty C project.

3.  Create a reference to the project.

    a) In the **Eclipse** menu, select **Run ->
       Debug Configurations -> C/C++Application -> Main**.

    b) Click the Project: **Browse** button and select the
       project you created a reference to.

    c) Click the C/C++Application: **Browse** button and select
       an existing ELF or binary file.

    d) Deselect **Enable auto build** and click **Apply**.

4.  Select the **Common** tab.

5.  In the **Save as:** field, type `Local file` and
    click **Apply**.

6.  Select the **Debugger** tab.

7.  In the **Executable:** field, type the path to the GDB installation.

8.  In the **Device name:** field, type `MK64FN1M0xxx12`
    and click **Apply**.

9.  Select the **Startup** tab.

10. Deselect **SWO Enable**.

11. Deselect **Enable semihosting**.

12. Select :guilabel:`Load symbols`.

13. Click **Use File** and type the name of a Zephyr
    .elf file.

14. Click **Apply**.

Programming Flash with J-link
=============================

Program Flash with J-Link to run the an image directly
from the shell.


Prerequisites
-------------

* Have the Zephyr application image file saved as a binary file.
  (The build should have created this binary file automatically.)


Steps
-----

1.  In a console, change directory to the J-Link installation directory.

2.  At the *J-Link>* prompt, enter::

       exec device = MK64FN1M0xxx12

3.  Enter::

       loadbin [filename], [addr]

    Example: ``loadbin zephyr.bin, 0x0``

4.  Enter::

       verifybin [filename],[addr]

    Example: ``verifybin zephyr.bin, 0x0``

5.  To reset the target, enter::

       r

6.  To start the image running directly from the shell, enter::

       g

7.  To stop the image from running, enter::

       h

Known Problems and Limitations
******************************

There is no support for the following:

* Memory protection through optional MPU.
  However, using a XIP kernel effectively provides
  TEXT/RODATA write protection in ROM.

* SRAM at addresses 0x1FFF0000-0x1FFFFFFF

* Writing to the hardware's flash memory

Bibliography
************

1. The Definitive Guide to the ARM Cortex-M3,
   Second Edition by Joseph Yiu (ISBN?978-0-12-382090-7)
2. ARMv7-M Architecture Technical Reference Manual
   (ARM DDI 0403D ID021310)
3. Procedure Call Standard for the ARM Architecture
   (ARM IHI 0042E, current through ABI release 2.09,
   2012/11/30)
4. Cortex-M3 Revision r2p1 Technical Reference Manual
   (ARM DDI 0337I ID072410)
5. Cortex-M4 Revision r0p1 Technical Reference Manual
   (ARM DDI 0439D ID061113)
6. Cortex-M3 Devices Generic User Guide
   (ARM DUI 0052A ID121610)
7. K64 Sub-Family Reference Manual, Rev. 2, January 2014
   (Freescale K64P144M120SF5RM)
8. FRDM-K64F Freedom Module User's Guide, Rev. 0, 04/2014
   (Freescale FRDMK64FUG)
