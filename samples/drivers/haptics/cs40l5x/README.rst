.. zephyr:code-sample:: cs40l5x
   :name: CS40L5x Haptic Driver
   :relevant-api: haptics_interface

   Drive an LRA using the CS40L5x haptic driver chip.

Overview
********

CS40L5x is a family (CS40L50/51/52/53) of haptics drivers designed for trackpads, tablets,
and automotive applications.

This sample demonstrates ROM features available on CS40L5x haptics drivers, including
ROM effects, custom PCM/PWLE effects, edge-triggered haptic effects, buzz effects,
and runtime haptics logging. A custom shell interface is provided that exposes a subset of
the CS40L5x API; basic demonstrations of other API calls are provided in main().

Building and Running
********************

Build the application for the :zephyr:board:`nucleo_f401re` board, and connect a CS40L5x haptic driver
on the bus I2C1 at the address 0x34. Connect three host GPIO pins to CS40L5x pins 11, 12, and 13 for
edge-triggered effects.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/haptics/cs40l5x
   :board: nucleo_f401re
   :goals: build
   :compact:

Alternatively, build the application for the :zephyr:board:`crd40l50` demonstration board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/haptics/cs40l5x
   :board: crd40l50
   :goals: build
   :compact:

For flashing the application, refer to the Flashing sections of the :zephyr:board:`nucleo_f401re` or
:zephyr:board:`crd40l50` board documentation.

.. code-block:: none

  *** Booting Zephyr OS build v4.3.0-rc1-387-gbf4e58c2f208 ***
  [00:00:00.085,000] <inf> cs40l5x_34.0: upload    | RTH 0 -> PCM
  [00:00:00.105,000] <inf> cs40l5x_34.0: upload    | RTH 1 -> PWLE
  [00:00:00.113,000] <inf> cs40l5x_34.0: configure | BUZ 0 -> 240 Hz, 10%, 0 ms
  [00:00:00.118,000] <inf> cs40l5x_34.0: configure | gpio@40020000 13 -> ROM 17 (-3 dB)
