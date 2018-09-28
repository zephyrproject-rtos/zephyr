.. _os-power-mgr-sample:

OS Power management demo
###########################

Overview
********

This sample demonstrates OS managed power saving mechanism through the sample
application which will periodically go sleep there by invoking the idle thread
which will call the _sys_soc_suspend() to enter into low power states. The Low
Power state will be selected based on the next timeout event.

Requirements
************

This application uses nrf52 DK board for the demo.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/power/power_mgr
   :board: nrf52_pca10040
   :goals: build flash
   :compact:

Running:

1. Open UART terminal.
2. Power Cycle Device.
3. Device will enter into Low Power Modes periodically.


Sample Output
=================
nrf52 core output
-----------------

.. code-block:: console

  ***OS Power Management Demo on arm****
  Demo Description
  Application creates Idleness, Due to which System Idle Thread is
  scheduled and it enters into various Low Power States.

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 6000 msec -->
  Entering Low Power state (0)
  Entering Low Power state (0)
  Entering Low Power state (0)
  Entering Low Power state (0)

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 11000 msec -->
  Entering Low Power state (1)
  Entering Low Power state (1)
  Entering Low Power state (1)
  Entering Low Power state (1)

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 6000 msec -->
  Entering Low Power state (0)
  Entering Low Power state (0)
  Entering Low Power state (0)
  Entering Low Power state (0)

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 11000 msec -->
  Entering Low Power state (1)
  Entering Low Power state (1)
  Entering Low Power state (1)
  Entering Low Power state (1)
  OS managed Power Management Test completed

