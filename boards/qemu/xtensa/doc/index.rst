.. zephyr:board:: qemu_xtensa

Overview
********

The QEMU board configuration is used to emulate the Xtensa architecture. This board
configuration provides support for the Xtensa simulation environment.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Use this configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment, for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_xtensa/dc233c
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 27 2017 13:09:26 *****
        threadA: Hello World from xtensa!
        threadB: Hello World from xtensa!
        threadA: Hello World from xtensa!
        threadB: Hello World from xtensa!
        threadA: Hello World from xtensa!
        threadB: Hello World from xtensa!
        threadA: Hello World from xtensa!
        threadB: Hello World from xtensa!
        threadA: Hello World from xtensa!
        threadB: Hello World from xtensa!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
