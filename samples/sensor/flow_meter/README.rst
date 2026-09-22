.. zephyr:code-sample:: flow-meter
   :name: Flow Meter
   :relevant-api: sensor_interface

   Read flow rate and volume from a Hall-effect pulse-output flow meter.

Overview
********

This sample demonstrates the flow meter driver. It shows how to:

- Poll the cumulative volume (``SENSOR_CHAN_VOLUME``) and
  the instantaneous flow rate (``SENSOR_CHAN_FLOW_RATE``, in L/min).
- Arm a one-shot volume threshold trigger using ``SENSOR_TRIG_THRESHOLD``
  and ``SENSOR_ATTR_UPPER_THRESH``, and receive a callback once the target
  volume has passed.

On ``native_sim`` the sample generates synthetic pulses via ``gpio_emul`` so
no physical sensor is required.  On real boards a Hall-effect flow meter must
be connected to the GPIO pin defined in the board overlay.

Building and Running
********************

Simulate on native_sim (no hardware required):

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/flow_meter
   :board: native_sim
   :goals: build run

Flash to an ESP32-C6 DevKit (flow meter on GPIO4 required):

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/flow_meter
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: build flash

Porting to Another Board
************************

Create a board overlay under ``boards/<board>.overlay`` that defines a
``flow_meter_0`` node with ``pulse-gpios`` pointing to the GPIO connected to
the sensor output, and set ``pulses-per-liter`` to the factory calibration
value.  The simulated pulse generation is compiled out automatically on
boards without ``CONFIG_GPIO_EMUL``.

Sample Output
*************

.. code-block:: console

   Flow meter sample
   1 L volume threshold armed

   volume=0.100000 L  rate=150.000000 L/min
   volume=0.200000 L  rate=10.909090 L/min
   volume=0.300000 L  rate=10.909090 L/min
   volume=0.400000 L  rate=10.909090 L/min
   volume=0.500000 L  rate=10.909090 L/min
   volume=0.600000 L  rate=10.909090 L/min
   volume=0.700000 L  rate=10.909090 L/min
   volume=0.800000 L  rate=10.909090 L/min
   volume=0.900000 L  rate=10.909090 L/min
   volume=1.000000 L  rate=10.909090 L/min

   Volume threshold reached! Total volume: 1.000000 L

The first rate reading is higher than subsequent ones because the initial
measurement interval starts at driver init (before the first pulse burst),
making it shorter than the steady 500 ms polling period.
