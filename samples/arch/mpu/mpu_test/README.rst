.. zephyr:code-sample:: mpu
   :name: Memory Protection Unit (MPU)

   Use memory protection to prevent common security issues.

Overview
********
This application provides a set options to check the correct MPU configuration
against the following security issues:

* Read at an address that is reserved in the memory map.
* Write into the boot Flash/ROM.
* Run code located in SRAM.

If the MPU configuration is correct each option selected ends up in an MPU
fault.

Building and Running
********************

This project can be built and executed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/arch/mpu/mpu_test
   :board: v2m_beetle
   :goals: build flash
   :compact:

To build the single thread version, use the supplied configuration file for
single thread: :file:`prj_single.conf`:

.. zephyr-app-commands::
   :zephyr-app: samples/arch/mpu/mpu_test
   :board: v2m_beetle
   :conf: prj_single.conf
   :goals: run
   :compact:

To build a version that allows writes to the flash device, edit
``prj.conf``, and follow the directions in the comments to enable the
proper configs.

Sample Output
=============

When running the above on an ARMv7 or ARMv8 CPU, the output of each command may look
like the following.

.. code-block:: console

  uart:~$ mpu read
  <err> os: ***** BUS FAULT *****
  <err> os:   Precise data bus error
  <err> os:   BFAR Address: 0x24000000
  <err> os: r0/a1:  0x00009a5c  r1/a2:  0x00000008  r2/a3:  0x20001aa8
  <err> os: r3/a4:  0x24000000 r12/ip:  0x00000000 r14/lr:  0x000029b7
  <err> os:  xpsr:  0x21000000
  <err> os: Faulting instruction address (r15/pc): 0x000003c8
  <err> os: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
  <err> os: Current thread: 0x20000708 (shell_uart)
  <err> os: Halting system


.. code-block:: console

  uart:~$ mpu write
  write address: 0x4000
  <err> os: ***** MPU FAULT *****
  <err> os:   Data Access Violation
  <err> os:   MMFAR Address: 0x4000
  <err> os: r0/a1:  0x00000000  r1/a2:  0x0000000e  r2/a3:  0x0badc0de
  <err> os: r3/a4:  0x00004000 r12/ip:  0x00000004 r14/lr:  0x000003ab
  <err> os:  xpsr:  0x61000000
  <err> os: Faulting instruction address (r15/pc): 0x000003b2
  <err> os: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
  <err> os: Current thread: 0x20000708 (shell_uart)
  <err> os: Halting system


.. code-block:: console

  uart:~$ mpu run
  <err> os: ***** MPU FAULT *****
  <err> os:   Instruction Access Violation
  <err> os: r0/a1:  0x00009a5c  r1/a2:  0x00000001  r2/a3:  0x20001aa8
  <err> os: r3/a4:  0x20000000 r12/ip:  0x00000000 r14/lr:  0x00006673
  <err> os:  xpsr:  0x60000000
  <err> os: Faulting instruction address (r15/pc): 0x20000000
  <err> os: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
  <err> os: Current thread: 0x20000708 (shell_uart)
  <err> os: Halting system


When running this test on an ARMv6 CPU (specifically on a Cortex-M0+), the output will
look different, as ARMv6 handles MPU faults as generic hard faults. Moreover, when
running the ``mpu run`` command, Zephyr's hard fault handler for AArch32 tries to
dereference the program counter from the exception stack frame, causing a second fault
and silently placing the processor into a lockup state.

To verify that the ``mpu run`` test in particular is running correctly, one can

* start the MPU test under gdb with ``west debug``
* execute ``mpu run`` over UART
* then interrupt gdb with Ctrl-C and show the program counter by running ``l`` in gdb.

The program counter should display as ``0xfffffffe``, indicating a lockup state.


.. code-block:: console

  uart:~$ mpu read
  <err> os: ***** HARD FAULT *****
  <err> os: r0/a1:  0x0800a54c  r1/a2:  0x00000008  r2/a3:  0x08003
  <err> os: r3/a4:  0x24000000 r12/ip:  0x00000040 r14/lr:  0x0800d
  <err> os:  xpsr:  0x01000000
  <err> os: Faulting instruction address (r15/pc): 0x08000486
  <err> os: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
  <err> os: Current thread: 0x200006a8 (shell_uart)
  <err> os: Halting system


.. code-block:: console

  uart:~$ mpu write
  <err> os: ***** HARD FAULT *****
  <err> os: r0/a1:  0x00000000  r1/a2:  0x0000000e  r2/a3:  0x0000e
  <err> os: r3/a4:  0x0badc0de r12/ip:  0x00000000 r14/lr:  0x08009
  <err> os:  xpsr:  0x61000000
  <err> os: Faulting instruction address (r15/pc): 0x0800046a
  <err> os: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
  <err> os: Current thread: 0x200006a8 (shell_uart)
  <err> os: Halting system


.. code-block:: console

  uart~$ mpu run
  <no output>
