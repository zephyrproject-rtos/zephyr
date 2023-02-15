.. _input-dump-sample:

Input Dump
##########

Overview
********

The Input Dump sample prints any input event using the :ref:`input` APIs.

Requirements
************

The samples works on any board with an input driver defined in the board devicetree.

Building and Running
********************

Build and flash as follows, changing ``nrf52dk_nrf52832`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/input/input_dump
   :board: nrf52dk_nrf52832
   :goals: build flash
   :compact:

After starting, the sample will print any input event in the console.
