.. _nrf5x-power-mgr-sample:

nRF5x Power management demo
###########################

Overview
********

This sample demonstrates a power manager app that uses the Zephyr
power management infrastructure to enter into Low Power state.

This app will cycle through the following power schemes each time the system
enters idle state:

1. Low Power State: Low Power State could be SoC, board and/or application
   specific. Being in this state is transparent to devices. SOC and devices
   do not lose context in this Mode. This example implements two Low Power
   states, which are signaled using LEDs on the development kit:

   A. LED1: [X], LED2: [X]: System is active, no low power state is selected.
   B. LED1: [X], LED2: [ ]: System is idle, and the SYS_POWER_STATE_SLEEP_2
      state is selected.
   C. LED1: [ ], LED2: [ ]: System is idle, and the SYS_POWER_STATE_SLEEP_3
      state is selected.

2. Deep Sleep: This Power State is mapped to SYSTEM OFF state. In this mode
   all devices on board get suspended. All devices and SOC lose context on
   wake up.

The power mode selection is done automatically by residency-based policy
implemented by the Zephyr Power Management Subsystem.

Requirements
************

This application uses nRF51 DK or nRF52 DK board for the demo.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf52/power_mgr
   :board: nrf52_pca10040
   :goals: build flash
   :compact:

Running:

1. Open UART terminal.
2. Power Cycle Device.
3. Device will enter into Low Power Modes.
4. Press Button 1 on device to enter deep sleep state.
5. Press Button 2 on device to wake up from deep sleep state.


Sample Output
=================
nRF52 core output
-----------------

.. code-block:: console

  *** Power Management Demo on nrf52_pca10040 ***
  Demo Description
  Application creates idleness, due to which System Idle Thread is
  scheduled and it enters into various Low Power States.

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 10 Sec -->
  --> Entering to SYS_POWER_STATE_SLEEP_2 state.
  --> Exited from SYS_POWER_STATE_SLEEP_2 state.

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 30 Sec -->
  --> Entering to SYS_POWER_STATE_SLEEP_3 state.
  --> Exited from SYS_POWER_STATE_SLEEP_3 state.

  <-- Disabling SYS_POWER_STATE_SLEEP_3 state --->

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 10 Sec -->
  --> Entering to SYS_POWER_STATE_SLEEP_2 state.
  --> Exited from SYS_POWER_STATE_SLEEP_2 state.

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 30 Sec -->
  --> Entering to SYS_POWER_STATE_SLEEP_2 state.
  --> Exited from SYS_POWER_STATE_SLEEP_2 state.

  <-- Enabling SYS_POWER_STATE_SLEEP_3 state --->
  <-- Disabling SYS_POWER_STATE_SLEEP_2 state --->

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 10 Sec -->

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 30 Sec -->
  --> Entering to SYS_POWER_STATE_SLEEP_3 state.
  --> Exited from SYS_POWER_STATE_SLEEP_3 state.

  <-- Enabling SYS_POWER_STATE_SLEEP_2 state --->
  <-- Forcing SYS_POWER_STATE_SLEEP_3 state --->

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 10 Sec -->
