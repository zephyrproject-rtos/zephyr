.. zephyr:board:: qemu_tc3x
.. zephyr:board:: qemu_tc4x

Overview
********

The TriCore QEMU board configurations are used to emulate the Infineon
AURIX TriCore architecture. Two boards are supported:

* ``qemu_tc3x`` - Emulates TC39x SoC (TriCore TC1.6.2P ISA)
* ``qemu_tc4x`` - Emulates TC4Dx SoC (TriCore TC1.8P ISA)

These board configurations provide support for the following devices:

* STM (System Timer Module)
* ASCLIN UART (polling mode)
* Clock control
* Interrupt router

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Known Problems or Limitations
=============================

* No SMP support
* No AMP support
* No MPU emulation

Programming and Debugging
*************************

QEMU Setup
==========

The TriCore QEMU is not yet integrated in the Zephyr SDK. It must be
built from source or downloaded as a prebuilt binary.

Prebuilt binaries are available at:
https://github.com/linumiz/qemu-tricore/releases

To build from source:

.. code-block:: console

   git clone https://github.com/linumiz/qemu-tricore.git
   cd qemu-tricore
   ./configure --target-list=tricore-softmmu
   make -j$(nproc) && make install

Ensure ``qemu-system-tricore`` is available in ``$PATH``.

.. zephyr:board-supported-runners::

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment. For example, with the :zephyr:code-sample:`hello_world` sample:

For TC39x (TC1.6.2P):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_tc3x
   :goals: run

For TC4Dx (TC1.8P):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_tc4x
   :goals: run

This will build an image with the hello_world sample, boot it using
QEMU, and display the following console output:

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-rc3 ***
   Hello World! qemu_tc4x/qemu_tc4x

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
