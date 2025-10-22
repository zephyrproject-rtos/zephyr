.. zephyr:code-sample:: sf32lb_codec
   :name: sf32lb_codec

   mic to speaker loopback.

Overview
********

A simple sample that can be used with any :ref:`supported board <sf32lb52_devkit_lcd>` and
mic to speaker loopback.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/driver/sf32lb_codec
   :host-os: unix
   :board: sf32lb52_devkit_lcd
   :goals: run
   :compact:

To build for another board, change "sf32lb52_devkit_lcd" above to that board's name.

Sample Output
=============

.. code-block:: console

    mic to speaker loopback.

