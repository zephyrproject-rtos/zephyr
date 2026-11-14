.. SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
..
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: qemu_rh850_u2a
.. zephyr:board:: qemu_rh850_u2b
.. zephyr:board:: qemu_rh850_u2c

Overview
********

Emulate Renesas RH850 microcontrollers under QEMU. Zephyr runs on CPU0 only.

* ``qemu_rh850_u2a``: RH850/U2A16 (R7F702300, G4MH core), QEMU machine
  ``rh850-u2a``, RLIN30 console.
* ``qemu_rh850_u2b``: RH850/U2B (G4MH core with FXU), QEMU machine
  ``rh850-u2b``, RLIN30 console.
* ``qemu_rh850_u2c``: RH850/U2C8 (R7F702600, G4KH core), QEMU machine
  ``rh850-u2c``, RLIN311 console.

Each board provides the OSTM0 system timer (tickless), an RLIN3 UART console
and the RH850 interrupt controller with nested interrupts (16 priority levels
on G4KH, 64 on G4MH). The FPU (single and double precision) is used with
``CONFIG_FPU`` and shared between threads with ``CONFIG_FPU_SHARING``. On
``qemu_rh850_u2b`` the FXU is enabled with ``CONFIG_FXU``: a thread gets the
FXU with its first FXU instruction, and FXU use in an interrupt is a fatal
error.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Known Problems or Limitations
=============================

* No user mode or MPU support, no thread local storage.
* No SMP or AMP: only CPU0 is used.
* Tested on QEMU only, not on silicon.
* picolibc has no ``setjmp``/``longjmp`` for the v850 target.

Programming and Debugging
*************************

QEMU Setup
==========

The Zephyr SDK does not ship RH850 support yet. A prebuilt
``qemu-system-rh850`` for x86_64 Linux hosts is available from the
qemu-rh850 v1.0.0 release,
https://github.com/linumiz/qemu-rh850/releases/tag/v1.0.0:

.. code-block:: console

   mkdir -p /opt/qemu-rh850
   tar -xzf qemu-rh850-linux-x86_64.tar.gz -C /opt/qemu-rh850

On other hosts, build it from the ``zephyr-qemu-rh850-v11.1.1`` branch of
https://github.com/linumiz/qemu-rh850:

.. code-block:: console

   git clone -b zephyr-qemu-rh850-v11.1.1 https://github.com/linumiz/qemu-rh850.git
   cd qemu-rh850
   ./configure --target-list=rh850-softmmu
   make -j$(nproc) && make install

Ensure ``qemu-system-rh850`` is on your ``$PATH``, or point ``QEMU_BIN_PATH``
to its directory.

Toolchain
=========

The Zephyr SDK does not ship an RH850 toolchain yet. Use the ``v850-elf`` GNU
toolchain (GCC 14.3.0, binutils 2.43.1 and GDB 16.2) from the
rh850-gcc-toolchain v1.0.0 release,
https://github.com/linumiz/rh850-gcc-toolchain/releases/tag/v1.0.0, and set:

.. code-block:: console

   mkdir -p /opt/toolchains/v850-elf
   tar -xzf v850-elf-toolchain-linux-x86_64.tar.gz -C /opt/toolchains/v850-elf
   export ZEPHYR_TOOLCHAIN_VARIANT=rh850
   export RH850_TOOLCHAIN_PATH=/opt/toolchains/v850-elf

.. zephyr:board-supported-runners::

Running
=======

Build and run the :zephyr:code-sample:`hello_world` sample (replace
``qemu_rh850_u2c`` by ``qemu_rh850_u2a`` or ``qemu_rh850_u2b`` for the other
boards):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_rh850_u2c
   :goals: run

The console shows:

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   Hello World! qemu_rh850_u2c/r7f702600

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
