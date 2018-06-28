.. _nrf52-power-mgr-sample:

nRF52 Power management demo
###########################

Overview
********

This sample demonstrates a power manager app that uses the Zephyr
power management infrastructure to enter into Low Power state.

This app will cycle through the following power schemes each time idle thread
calls _sys_soc_suspend() hook function :

1. Low Power State: Low Power State is SOC specific and being in this state is
   transparent to devices. SOC and devices do not lose context in this Mode.
   SOC supports the following two Low Power states :

   A. CONSTANT LATENCY LOW POWER MODE : This Low Power State triggers CONSTLAT
      task on nrf52 SOC. In this Mode there is Low Exit latency and 16MHz Clock
      is kept ON.
   B. LOW POWER STATE : In this Power State LOWPWR task is triggered on nrf52
      SOC. This is a deeper power state than CONSTANT LATENCY Mode. In this mode
      the 16MHz Clock is turned off and only 32KHz clock is used.

2. Deep Sleep: This Power State is mapped to SYSTEM OFF state. In this mode
   all devices on board get suspended. All devices and SOC lose context on
   wake up.

3. No-op: No operation, letting kernel idle.

Requirements
************

This application uses nrf52 DK board for the demo.

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
nrf52 core output
-----------------

.. code-block:: console

  ***Power Management Demo on arm****
  Demo Description
  Application creates Idleness, Due to which System Idle Thread is
  scheduled and it enters into various Low Power States.

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 30 Sec -->
  --> Entering into Constant Latency State --- Low Power State exit !
  --> Entering into Constant Latency State --- Low Power State exit !

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 90 Sec -->
  --> Entering into Low Power State --- Low Power State exit !
  --> Entering into Low Power State --- Low Power State exit !

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 30 Sec -->
  --> Entering into Constant Latency State --- Low Power State exit !
  --> Entering into Constant Latency State --- Low Power State exit !

  <-- App doing busy wait for 10 Sec -->

  <-- App going to sleep for 90 Sec -->
  --> Entering into Low Power State --- Low Power State exit !
  --> Entering into Low Power State --- Low Power State exit !

  Press BUTTON1 to enter into Deep Dleep state...Press BUTTON2 to exit Deep Sleep state
  --> Entering into Deep Sleep State -
