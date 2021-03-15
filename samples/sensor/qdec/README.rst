.. _qdec:

QDEC: Quadrature Decoder
###########################

Overview
********
A simple quadrature decoder example

Building and Running
********************

This project writes the quadrature decoder position to the console once every
2 seconds.

Building on SAM E70 Xplained board
==================================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/qdec
   :host-os: unix
   :board: sam_e70_xplained
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Quadrature Decoder sample application

    Position is 6
    Position is 12
    Position is -45

    <repeats endlessly every 2 seconds>
