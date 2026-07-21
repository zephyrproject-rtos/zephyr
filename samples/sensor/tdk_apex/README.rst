.. zephyr:code-sample:: tdk_apex
   :name: TDK Advanced Pedometer and Event Detection (APEX)
   :relevant-api: sensor_interface

   Get TDK APEX event detection (trigger mode).

Overview
********

This sample application shows how to use the APEX (Advanced Pedometer
and Event Detection) features of TDK Invensense sensors. It consists of:
** Pedometer: Tracks step count, and provide details such as the cadence
and the estimated activity type (Walk, Run, Unknown).
** Tilt Detection: Detects the Tilt when tilting the board with an angle
of 30 degrees or more. The tilt event is generated when the
position is held for 4 seconds.
** Wake on Motion (WoM): Detects motion per axis exceeding 195 mg threshold.
** Significant Motion Detector (SMD): Detects when the user has moved
significantly.
APEX features support are configured through devicetree.

References
**********

 - https://invensense.tdk.com/download-pdf/an-000271-icm-42607x-and-icm-42670x-apex-motion-functions-description-and-usage/

Driver configuration
********************

The APEX is based on accelerometer data only. The TDK Sensor driver configures
accelerometer low power mode and the APEX operating frequency (25Hz or 50Hz).

Wiring
*******

This sample uses an external breakout for the sensor. A devicetree
overlay must be provided to identify the TDK sensor, the SPI or I2C bus interface and the interrupt
sensor GPIO.

Building and Running
********************

This sample supports TDK IMU devices. Each device needs
to be aliased as ``tdk-apex-sensorN`` where ``N`` goes from ``0`` to ``9``. For example:

.. code-block:: devicetree

 / {
	aliases {
			tdk-apex-sensor0 = &icm42670p;
		};
	};

This sample supports APEX feature of TDK device. It needs to be specified as below:

.. code-block:: devicetree

	icm42670p: icm42670p@0 {
		apex = "pedometer";
	}

Make sure the apex feature used is in devicetree, then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/tdk_apex
   :board: nrf52dk/nrf52832
   :goals: build flash

Sample Output
=============

.. code-block:: devicetree

	icm42670p: icm42670p@0 {
		apex = "pedometer";
	}

.. code-block:: console

   Found device "icm42670p@68", getting sensor data
   [0:00:09.287]: STEP_DET     count: 6 steps  cadence: 2.0 steps/s  activity: Unknown
   [0:00:09.689]: STEP_DET     count: 7 steps  cadence: 2.1 steps/s  activity: Walk
   [0:00:10.051]: STEP_DET     count: 8 steps  cadence: 2.2 steps/s  activity: Walk
   [0:00:10.433]: STEP_DET     count: 9 steps  cadence: 2.2 steps/s  activity: Walk
   [0:00:10.835]: STEP_DET     count: 10 steps  cadence: 2.3 steps/s  activity: Walk

   <repeats endlessly>

.. code-block:: devicetree

	icm42670p: icm42670p@0 {
		apex = "tilt";
	}

.. code-block:: console

   Found device "icm42670p@68", getting sensor data
   [0:00:15.249]: TILT
   [0:00:21.479]: TILT
   [0:00:26.765]: TILT

   <repeats endlessly>

.. code-block:: devicetree

	icm42670p: icm42670p@0 {
		apex = "wom";
	}

.. code-block:: console

   Found device "icm42670p@68", getting sensor data
   [0:00:02.555]: WOM x=1 y=0 z=1
   [0:00:02.636]: WOM x=0 y=0 z=1
   [0:00:02.797]: WOM x=0 y=1 z=0
   [0:00:02.877]: WOM x=0 y=0 z=1
   [0:00:02.957]: WOM x=1 y=1 z=1

   <repeats endlessly>

.. code-block:: devicetree

	icm42670p: icm42670p@0 {
		apex = "smd";
	}

.. code-block:: console

   Found device "icm42670@68", getting sensor data
   [0:00:04.622]: SMD
   [0:00:05.084]: SMD
   [0:00:05.566]: SMD

   <repeats endlessly>
