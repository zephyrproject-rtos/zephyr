.. _watchdog-sample:

Watchdog Sample
###############

Overview
********

This sample demonstrates how to use the watchdog driver API.

A typical use case for a watchdog is that the board is restarted in case some piece of code
is kept in an infinite loop.

Building and Running
********************

In this sample, a watchdog callback is used to handle a timeout event once. This functionality is used to request an action before the board
restarts due to a timeout event in the watchdog driver.

The watchdog peripheral is configured in the board's ``.dts`` file. Make sure that the watchdog is enabled
using the configuration file in ``boards`` folder.

Building and Running for ST Nucleo F091RC
=========================================

The sample can be built and executed for the
:ref:`nucleo_f091rc_board` as follows:

.. zephyr-app-commands::
	:zephyr-app: samples/drivers/watchdog
	:board: nucleo_f091rc
	:goals: build flash
	:compact:

To build for another board, change "nucleo_f091rc" to the name of that board and provide a corresponding devicetree overlay.

Sample output
=============

You should get a similar output as below:

.. code-block:: console

	Watchdog sample application
	Attempting to test pre-reset callback
	Feeding watchdog 5 times
	Feeding watchdog...
	Feeding watchdog...
	Feeding watchdog...
	Feeding watchdog...
	Feeding watchdog...
	Waiting for reset...
	Handled things..ready to reset

.. note:: After the last message, the board will reset and the sequence will start again
