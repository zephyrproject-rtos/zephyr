.. _gdbstub:

GDB stub
########

.. contents::
   :local:
   :depth: 2

Overview
********

The gdbstub feature provides an implementation of the GDB Remote
Serial Protocol (RSP) that allows you to remotely debug Zephyr
using GDB.

The protocol supports different connection types: serial, UDP/IP and
TCP/IP. Zephyr currently supports only serial device communication.

The GDB program acts as the client while Zephyr acts as the
server. When this feature is enabled, Zephyr stops its execution after
:c:func:`gdb_init` starts gdbstub service and waits for a GDB
connection. Once a connection is established it is possible to
synchronously interact with Zephyr. Note that currently it is not
possible to asynchronously send commands to the target.

Features
********

The following features are supported:

* Add and remove breakpoints
* Continue and step the target
* Print backtrace
* Read or write general registers
* Read or write the memory

Enabling GDB Stub
*****************

GDB stub can be enabled with the :kconfig:option:`CONFIG_GDBSTUB` option.

Using Serial Backend
====================

The serial backend for GDB stub can be enabled with
the :kconfig:option:`CONFIG_GDBSTUB_SERIAL_BACKEND` option.

Since serial backend utilizes UART devices to send and receive GDB commands,

* If there are spare UART devices on the board, set ``zephyr,gdbstub-uart``
  property of the chosen node to the spare UART device so that :c:func:`printk`
  and log messages are not being printed to the same UART device used for GDB.

* For boards with only one UART device, :c:func:`printk` and logging
  must be disabled if they are also using the same UART device for output.
  GDB related messages may interleave with log messages which may have
  unintended consequences. Usually this can be done by disabling
  :kconfig:option:`CONFIG_PRINTK` and :kconfig:option:`CONFIG_LOG`.

Debugging
*********

Using Serial Backend
====================

#. Build with GDB stub and serial backend enabled.

#. Flash built image onto board and reset the board.

   * Execution should now be paused at :c:func:`gdb_init`.

#. Execute GDB on development machine and connect to the GDB stub.

   .. code-block:: bash

      target remote <serial device>

   For example,

   .. code-block:: bash

      target remote /dev/ttyUSB1

#. GDB commands can be used to start debugging.

Example
*******

This is an example using ``samples/subsys/debug/gdbstub`` to demonstrate
how GDB stub works.

#. Open two terminal windows.

#. On the first terminal, build and run the sample:

   .. zephyr-app-commands::
      :zephyr-app: samples/subsys/debug/gdbstub
      :host-os: unix
      :board: qemu_x86
      :goals: build run

#. On the second terminal, start GDB:

   .. code-block:: bash

      <SDK install directory>/x86_64-zephyr-elf/bin/x86_64-zephyr-elf-gdb

   #. Tell GDB where to look for the built ELF file:

      .. code-block:: text

         (gdb) file <build directory>/zephyr/zephyr.elf

      Response from GDB:

      .. code-block:: text

         Reading symbols from <build directory>/zephyr/zephyr.elf...

   #. Tell GDB to connect to the server:

      .. code-block:: text

         (gdb) target remote localhost:5678

      Note that QEMU is setup to redirect the serial used for GDB stub in
      the Zephyr image to a networking port. Hence the connection to
      localhost, port 5678.

      Response from GDB:

      .. code-block:: text

         Remote debugging using :5678
         arch_gdb_init () at <ZEPHYR_BASE>/arch/x86/core/ia32/gdbstub.c:232
         232     }

      GDB also shows where the code execution is stopped. In this case,
      it is at :file:`arch/x86/core/ia32/gdbstub.c`, line 232.

   #. Use command ``bt`` or ``backtrace`` to show the backtrace of stack frames.

      .. code-block:: text

         (gdb) bt
         #0  arch_gdb_init () at <ZEPHYR_BASE>/arch/x86/core/ia32/gdbstub.c:232
         #1  0x00105068 in gdb_init (arg=0x0) at <ZEPHYR_BASE>/subsys/debug/gdbstub.c:833
         #2  0x00109d6f in z_sys_init_run_level (level=0x1) at <ZEPHYR_BASE>/kernel/device.c:72
         #3  0x0010a40b in z_cstart () at <ZEPHYR_BASE>/kernel/init.c:423
         #4  0x00105383 in z_x86_prep_c (arg=0x9500) at <ZEPHYR_BASE>/arch/x86/core/prep_c.c:58
         #5  0x001000a9 in __csSet () at <ZEPHYR_BASE>/arch/x86/core/ia32/crt0.S:273

   #. Use command ``list`` to show the source code and surroundings where
      code execution is stopped.

      .. code-block:: text

         (gdb) list
         227     }
         228
         229     void arch_gdb_init(void)
         230     {
         231             __asm__ volatile ("int3");
         232     }
         233
         234     /* Hook current IDT. */
         235     _EXCEPTION_CONNECT_NOCODE(z_gdb_debug_isr, IV_DEBUG, 3);
         236     _EXCEPTION_CONNECT_NOCODE(z_gdb_break_isr, IV_BREAKPOINT, 3);

   #. Use command ``s`` or ``step`` to step through program until it reaches
      a different source line. Now that it finished executing :c:func:`arch_gdb_init`
      and is continuing in :c:func:`gdb_init`.

      .. code-block:: text

         (gdb) s
         gdb_init (arg=0x0) at /home/dleung5/zephyr/rtos/zephyr/subsys/debug/gdbstub.c:834
         834     return 0;

      .. code-block:: text

         (gdb) list
         829                     LOG_ERR("Could not initialize gdbstub backend.");
         830                     return -1;
         831             }
         832
         833             arch_gdb_init();
         834             return 0;
         835     }
         836
         837     #ifdef CONFIG_XTENSA
         838     /*

   #. Use command ``br`` or ``break`` to setup a breakpoint. This example
      sets up a breakpoint at :c:func:`main`, and let code execution continue
      without any intervention using command ``c`` (or ``continue``).

      .. code-block:: text

         (gdb) break main
         Breakpoint 1 at 0x1005a9: file <ZEPHYR_BASE>/samples/subsys/debug/gdbstub/src/main.c, line 32.
         (gdb) continue
         Continuing.

      Once code execution reaches :c:func:`main`, execution will be stopped
      and GDB prompt returns.

      .. code-block:: text

         Breakpoint 1, main () at <ZEPHYR_BASE>/samples/subsys/debug/gdbstub/src/main.c:32
         32           ret = test();

      Now GDB is waiting at the beginning of :c:func:`main`:

      .. code-block:: text

         (gdb) list
         27
         28     void main(void)
         29     {
         30             int ret;
         31
         32             ret = test();
         33             printk("%d\n", ret);
         34     }
         35
         36     K_THREAD_DEFINE(thread, STACKSIZE, thread_entry, NULL, NULL, NULL,

   #. To examine the value of ``ret``, the command ``p`` or ``print``
      can be used.

      .. code-block:: text

         (gdb) p ret
         $1 = 0x11318c

      Since ``ret`` has not been assigned a value yet, what it contains is
      simply a random value.

   #. If step (``s`` or ``step``) is used here, it will continue execution
      until :c:func:`printk` is reached, thus skipping the interior of
      :c:func:`test`. To examine code execution inside :c:func:`test`,
      a breakpoint can be set for :c:func:`test`, or simply using
      ``si`` (or ``stepi``) to execute one machine instruction, where it has
      the side effect of going into the function.

      .. code-block:: text

         (gdb) si
         test () at <ZEPHYR_BASE>/samples/subsys/debug/gdbstub/src/main.c:13
         13     {
         (gdb) list
         8      #include <zephyr/sys/printk.h>
         9
         10     #define STACKSIZE 512
         11
         12     static int test(void)
         13     {
         14             int a;
         15             int b;
         16
         17             a = 10;

   #. Here, ``step`` can be used to go through all code inside :c:func:`test`
      until it returns. Or the command ``finish`` can be used to continue
      execution without intervention until the function returns.

      .. code-block:: text

         (gdb) finish
         Run till exit from #0  test () at <ZEPHYR_BASE>/samples/subsys/debug/gdbstub/src/main.c:13
         0x001005ae in main () at <ZEPHYR_BASE>/samples/subsys/debug/gdbstub/src/main.c:32
         32             ret = test();
         Value returned is $2 = 0x1e

      And now, execution is back to :c:func:`main`.

   #. Examine ``ret`` again which should have the return value from
      :c:func:`test`. Sometimes, the assignment is not done until another
      ``step`` is issued, as in this case. This is due to the assignment
      code is done after returning from function. The assignment code is
      generated by the toolchain as machine instructions which are not
      visible when viewing the corresponding C source file.

      .. code-block:: text

         (gdb) p ret
         $3 = 0x11318c
         (gdb) s
         33              printk("%d\n", ret);
         (gdb) p ret
         $4 = 0x1e

   #. If ``continue`` is issued here, code execution will continue indefinitely
      as there are no breakpoints to further stop execution. Breaking execution
      in GDB via Ctrl-C does not currently work as the GDB stub does not
      support this functionality (yet).
