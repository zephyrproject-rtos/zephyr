.. zephyr:code-sample:: battery_mgmt
   :name: Advanced Battery Management Device with Charger, Fuel Gauge, and Regulators
   :relevant-api: charger_interface fuel_gauge_interface regulator_interface

   Implements advanced battery management using charger,
   fuel gauge, and regulator subsystems.

Overview
********

This sample demonstrates how to use an advanced battery management device with
Zephyr's driver subsystems. The sample is designed to work with any device or set of devices
that provides a charger, fuel gauge, and regulator functionality through the standard
Zephyr device driver interfaces.

The sample shows how to:

* Monitor battery charging status and health using the :ref:`Charger API <charger_api>`
* Read battery voltage, state of charge, and design capacity
  using the :ref:`Fuel Gauge API <fuel_gauge_api>`
* Control and monitor voltage regulators using the :ref:`Regulator API <regulator_api>`

The application continuously polls the charger and fuel gauge to display real-time
battery and charging information, including:

* Charger configuration (current limits, charge currents, voltages)
* Charging status (online, present, charging state, health)
* Battery fuel gauge data (voltage, state of charge, design capacity)
* Regulator output voltages

Requirements
************

The sample requires a board with an advanced battery management device that
supports charger, fuel gauge, and regulator functionality. The device must be
defined in the devicetree with appropriate labels so that the sample code can
access the individual subsystems.

The devicetree must include an ``/aliases`` node with the following entries:

* ``charger`` pointing to a node implementing the charger interface
* ``fuelgauge`` pointing to a node implementing the fuel gauge interface
* ``buck`` pointing to a buck regulator node
* ``buckboost`` pointing to a buck-boost regulator node

Example:

.. code-block:: devicetree

   / {
           aliases {
                   charger = &charger;
                   fuelgauge = &fuelgauge;
                   buck = &buck;
                   buckboost = &buckboost;
           };
   };

An example overlay is provided for the ``nucleo_u575zi_q`` board showing how to
configure these labels, using the ``ADP5360`` as a reference device.

Hardware Setup
==============

The specific hardware connections depend on the device being used. Refer to the
device datasheet and the overlay file for your board for detailed connection
instructions.

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/battery_mgmt`.

To build and flash the application for the verified board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/battery_mgmt
   :board: nucleo_u575zi_q
   :goals: build flash
   :compact:

To use a different board, create a file in the ``boards/`` subdirectory named
``<board>.overlay`` with the appropriate devicetree configuration for your
advanced battery management device.

Sample Output
=============

The sample output will vary based on the specific device and configuration.
A typical output showing battery charging and monitoring information might look like:

.. code-block:: console

   [0:00:00.003]: Found device "charger", getting charger data
   [0:00:00.006]: Current (Fast Charge): 20000 uA
   [0:00:00.009]: Adaptive Voltage: 3800000 uV
   [0:00:00.012]: Current (Termination): 5000 uA
   [0:00:03.024]: Buck Output Voltage: 1300000
   [0:00:03.027]: Buck Boost Output Voltage: 3300000
   [0:00:03.030]: Online: Charger is active.
   [0:00:03.033]: Present: Battery is Present.
   [0:00:03.036]: Status: Charger is Charging.
   [0:00:03.039]: Mode: Charging is in Fast Mode (CC).
   [0:00:03.042]: Health: Battery is Cool.
   [0:00:03.045]: Fuel gauge Voltage: 3950000
   [0:00:03.048]: Fuel gauge Status 0x0
   [0:00:03.051]: Fuel gauge SoC: 75
