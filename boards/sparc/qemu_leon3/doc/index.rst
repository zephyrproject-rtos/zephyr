.. _qemu_leon3:

LEON3 Emulation (QEMU)
######################

Overview
********

The LEON3 QEMU board configuration is used to emulate the LEON3 processor.

Programming and Debugging
*************************

Applications for the ``qemu_leon3`` board configuration can be built and run in
the usual way for emulated boards (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment. For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_leon3
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v2.4.0-27-g7b37fdd5303b  ***
        threadA: Hello World from qemu_leon3!
        threadB: Hello World from qemu_leon3!
        threadA: Hello World from qemu_leon3!
        threadB: Hello World from qemu_leon3!
        threadA: Hello World from qemu_leon3!
        threadB: Hello World from qemu_leon3!
        threadA: Hello World from qemu_leon3!
        threadB: Hello World from qemu_leon3!
        threadA: Hello World from qemu_leon3!
        threadB: Hello World from qemu_leon3!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
