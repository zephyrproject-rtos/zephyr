.. _event_manager_sample:

Event Manager
#############

.. contents::
   :local:
   :depth: 2

The Event Manager sample demonstrates the functionality of the :ref:`event_manager` subsystem.
It uses an event-driven architecture, where different modules communicate through sending and processing events.


Overview
********

The sample application consists of three modules that communicate using events:

Sensor (``sensor_simulated.c``):
  This module waits for a configuration event (which is sent by ``main.c``).
  After receiving this event, it simulates measured data at constant intervals.
  Every time the data is updated, the module sends the current values as measurement event.
  When the module receives a control event from the Controller, it responds with an ACK event.

Controller (``controller.c``):
  This module waits for measurement events from the sensor.
  Every time a measurement event is received, the module checks one of the measurement values that are transmitted as part of the event and, if the value exceeds a static threshold, sends a control event.

Statistics (``stats.c``):
  This module waits for measurement events from the sensor.
  The module calculates and logs basic statistics about one of the measurement values that are transmitted as part of the event.

Building and testing
********************

1.Using development board:
  Build and flash Event Manager as follows, changing nrf52840dk_nrf52840 for your board:
  west build -b nrf52840dk_nrf52840 samples/subsys/event_manager
  west flash
  Then connect to the kit with a terminal emulator (for example, PuTTY).
#. Using qemu
  If you use qemu platform changing qemu_leon3 for your board:
  west build -b quemu_leon3 samples/subsys/event_manager
  west build -t run
#. Observe that output similar to the following is logged on UART in case of development kit and in terminal in case of qemu::

      ***** Booting Zephyr OS v1.13.99-ncs1-4741-g1d6219f *****
      [00:00:00.000,854] <inf> event_manager: e: config_event init_val_1=3
      [00:00:00.001,068] <inf> event_manager: e: measurement_event val1=3 val2=3 val3=3
      [00:00:00.509,063] <inf> event_manager: e: measurement_event val1=3 val2=6 val3=9
      [00:00:01.018,005] <inf> event_manager: e: measurement_event val1=3 val2=9 val3=18
      [00:00:01.526,947] <inf> event_manager: e: measurement_event val1=3 val2=12 val3=30
      [00:00:02.035,888] <inf> event_manager: e: measurement_event val1=3 val2=15 val3=45
      [00:00:02.035,949] <inf> event_manager: e: control_event
      [00:00:02.035,980] <inf> event_manager: e: ack_event
      [00:00:02.544,830] <inf> event_manager: e: measurement_event val1=-3 val2=12 val3=57
      [00:00:03.053,771] <inf> event_manager: e: measurement_event val1=-3 val2=9 val3=66
      [00:00:03.562,713] <inf> event_manager: e: measurement_event val1=-3 val2=6 val3=72
      [00:00:04.071,655] <inf> event_manager: e: measurement_event val1=-3 val2=3 val3=75
      [00:00:04.580,596] <inf> event_manager: e: measurement_event val1=-3 val2=0 val3=75
      [00:00:04.580,596] <inf> stats: Average value3: 45


Dependencies
************

This sample uses the following Zephyr subsystems:

* :ref:`event_manager`
* :ref:`logging_api`
