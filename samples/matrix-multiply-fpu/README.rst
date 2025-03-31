.. _matrix_multiply:

Matrix-Multiply
###############

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
prints the values after the Matrix Multiplication of the numbers on to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/matrix-multiply
   :host-os: unix
   :board: secure_iot
   :goals: run
   :compact:

To build for another board, change "secure_iot" above to that board's name.

Sample Output
=============

.. code-block:: console

    Output of the Matrix Multiplication

If running in QEMU, exit from QEMU by pressing :kbd:`CTRL+C` :kbd:`x`.
