.. _mpu_test:

Memory Protection Unit (MPU) Sample
###################################

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
   :zephyr-app: samples/mpu/mpu_test
   :board: v2m_beetle
   :goals: build flash
   :compact:

To build the single thread version, use the supplied configuration file for
single thread: :file:`prj_single.conf`:

.. zephyr-app-commands::
   :zephyr-app: samples/mpu/mpu_test
   :board: v2m_beetle
   :conf: prj_single.conf
   :goals: run
   :compact:

To build a version that allows writes to the flash device, edit
``prj.conf``, and follow the directions in the comments to enable the
proper configs.

Sample Output
=============

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
