.. _pxp_demo:

PXP demo
########

Overview
********

some samples that can be used with any :ref:`supported board <boards>` and
to show how the pxp works.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/pxp
   :host-os: unix
   :board: qemu_x86
   :goals: build
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

    Convert Done!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
