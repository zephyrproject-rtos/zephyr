.. SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
.. SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
.. SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
..
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: qemu_tc3x
.. zephyr:board:: qemu_tc4x

Overview
********

Emulate the Infineon AURIX TriCore architecture under QEMU. Two boards are
supported:

* ``qemu_tc3x`` - TC39x SoC (TriCore TC1.6.2P ISA)
* ``qemu_tc4x`` - TC4Dx SoC (TriCore TC1.8P ISA)

Both provide the STM system timer, a polling-mode ASCLIN UART console and the
interrupt router.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Known Problems or Limitations
=============================

* No SMP, AMP or MPU emulation

Programming and Debugging
*************************

QEMU Setup
==========

TriCore QEMU is not part of the Zephyr SDK. Use a prebuilt binary from
https://github.com/linumiz/qemu-tricore/releases, or build it from source:

.. code-block:: console

   git clone https://github.com/linumiz/qemu-tricore.git
   cd qemu-tricore
   ./configure --target-list=tricore-softmmu
   make -j$(nproc) && make install

Ensure ``qemu-system-tricore`` is on your ``$PATH``.

.. zephyr:board-supported-runners::

Running
=======

Build and run the :zephyr:code-sample:`hello_world` sample (use ``qemu_tc4x``
for the TC4Dx target):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_tc3x
   :goals: run

The console shows:

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   Hello World! qemu_tc3x/qemu_tc3x

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
