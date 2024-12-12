.. zephyr:code-sample:: stream_fifo
   :name: Generic device FIFO streaming
   :relevant-api: sensor_interface

   Get accelerometer/gyroscope/temperature FIFO data frames from a sensor using
   SENSOR_TRIG_FIFO_WATERMARK as a trigger.

Overview
********

This sample application demonstrates how to stream FIFO data using the `RTIO framework`_.

The streaming is started using the sensor_stream() API and it is self-sustained by the
SENSOR_TRIG_FIFO_WATERMARK trigger.

Currently the sample gets/prints data for the following sensor channels:

        - SENSOR_CHAN_ACCEL_XYZ
        - SENSOR_CHAN_GYRO_XYZ
        - SENSOR_CHAN_DIE_TEMP

Building and Running
********************

This sample supports up to 10 FIFO streaming devices. Each device needs
to be aliased as ``streamN`` where ``N`` goes from ``0`` to ``9``. For example:

.. code-block:: devicetree

  / {
      aliases {
                   stream0 = &lsm6dsv16x_6b_x_nucleo_iks4a1;
              };
      };

Example devicetree overlays and configurations are already available for nucleo_f401re and
nucleo_h503rb in the boards directory:

- :zephyr_file:`samples/sensor/stream_fifo/boards/nucleo_f401re.overlay`

  DT overlay file for the nucleo_f401re board.

- :zephyr_file:`samples/sensor/stream_fifo/boards/nucleo_f401re.conf`

  Configuration file for the nucleo_f401re board.

- :zephyr_file:`samples/sensor/stream_fifo/boards/nucleo_h503rb.overlay`

  DT overlay file for the nucleo_h503rb board.

- :zephyr_file:`samples/sensor/stream_fifo/boards/nucleo_h503rb.conf`

  Configuration file for the nucleo_h503rb board.

For example, build and run sample for nucleo_h503rb with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/stream_fifo
   :board: nucleo_h503rb
   :goals: build flash
   :compact:

Sample Output
=============

The following example output is for a lsm6dsv16x IMU device with accelerometer, gyroscope
and temperature sensors. The FIFO watermark is set to 64. The board used is a nucleo_h503rb
equipped with a `x-nucleo-iks4a1`_ shield.

.. code-block:: console

       FIFO count - 65
       XL data for lsm6dsv16x@6b 708054166675ns (0.267959, -0.162689, 9.933659)
       XL data for lsm6dsv16x@6b 708087500008ns (0.258389, -0.162689, 9.919304)
       XL data for lsm6dsv16x@6b 708120833341ns (0.253604, -0.167474, 9.909734)
       XL data for lsm6dsv16x@6b 708154166674ns (0.258389, -0.153119, 9.933659)
       XL data for lsm6dsv16x@6b 708187500007ns (0.253604, -0.157904, 9.919304)
       XL data for lsm6dsv16x@6b 708220833340ns (0.258389, -0.162689, 9.943229)
       XL data for lsm6dsv16x@6b 708254166673ns (0.258389, -0.157904, 9.904949)
       XL data for lsm6dsv16x@6b 708287500006ns (0.253604, -0.162689, 9.952799)
       GY data for lsm6dsv16x@6b 708054166675ns (0.000609, 0.000000, 0.004574)
       GY data for lsm6dsv16x@6b 708087500008ns (-0.000304, 0.000304, 0.003964)
       GY data for lsm6dsv16x@6b 708120833341ns (-0.000609, 0.000304, 0.004269)
       GY data for lsm6dsv16x@6b 708154166674ns (0.000304, -0.000304, 0.004879)
       GY data for lsm6dsv16x@6b 708187500007ns (0.001219, -0.001524, 0.004269)
       GY data for lsm6dsv16x@6b 708220833340ns (-0.000914, -0.000609, 0.004879)
       GY data for lsm6dsv16x@6b 708254166673ns (0.000000, -0.000304, 0.004269)
       GY data for lsm6dsv16x@6b 708287500006ns (0.000609, 0.000000, 0.004574)
       TP data for lsm6dsv16x@6b 708087500008ns 24.390625 °C
       TP data for lsm6dsv16x@6b 708154166674ns 24.421875 °C
       TP data for lsm6dsv16x@6b 708220833340ns 24.414062 °C
       TP data for lsm6dsv16x@6b 708287500006ns 24.441406 °C
       XL data for lsm6dsv16x@6b 708320833339ns (0.258389, -0.157904, 9.957584)
       XL data for lsm6dsv16x@6b 708354166672ns (0.258389, -0.177044, 9.900164)
       XL data for lsm6dsv16x@6b 708387500005ns (0.019139, 0.277529, 9.426449)
       XL data for lsm6dsv16x@6b 708420833338ns (-0.052634, -0.244034, 9.866669)
       XL data for lsm6dsv16x@6b 708454166671ns (0.263174, -0.172259, 9.861884)
       XL data for lsm6dsv16x@6b 708487500004ns (0.277529, -0.129194, 9.890594)
       XL data for lsm6dsv16x@6b 708520833337ns (0.301454, -0.138764, 9.928874)
       XL data for lsm6dsv16x@6b 708554166670ns (0.258389, -0.138764, 9.971939)
       GY data for lsm6dsv16x@6b 708320833339ns (0.000304, 0.000609, 0.004269)
       GY data for lsm6dsv16x@6b 708354166672ns (0.001524, -0.003049, 0.004269)
       GY data for lsm6dsv16x@6b 708387500005ns (-0.015554, -0.025314, 0.004879)
       GY data for lsm6dsv16x@6b 708420833338ns (0.006404, 0.006099, 0.011894)
       GY data for lsm6dsv16x@6b 708454166671ns (-0.004879, 0.007014, -0.001829)
       GY data for lsm6dsv16x@6b 708487500004ns (0.014944, -0.023789, 0.003354)
       GY data for lsm6dsv16x@6b 708520833337ns (0.008539, -0.017689, 0.006099)
       GY data for lsm6dsv16x@6b 708554166670ns (0.005489, -0.004574, 0.005184)
       TP data for lsm6dsv16x@6b 708354166672ns 24.339843 °C
       TP data for lsm6dsv16x@6b 708420833338ns 24.375000 °C
       TP data for lsm6dsv16x@6b 708487500004ns 24.421875 °C
       TP data for lsm6dsv16x@6b 708554166670ns 24.398437 °C
       XL data for lsm6dsv16x@6b 708587500003ns (0.272744, -0.172259, 9.861884)
       XL data for lsm6dsv16x@6b 708620833336ns (0.344519, -0.267959, 10.038929)
       XL data for lsm6dsv16x@6b 708654166669ns (0.339734, -0.081344, 9.967154)
       XL data for lsm6dsv16x@6b 708687500002ns (0.263174, -0.124409, 9.981509)
       XL data for lsm6dsv16x@6b 708720833335ns (0.296669, -0.181829, 9.948014)
       XL data for lsm6dsv16x@6b 708754166668ns (0.272744, -0.114839, 9.948014)
       XL data for lsm6dsv16x@6b 708787500001ns (0.296669, -0.153119, 9.995864)
       XL data for lsm6dsv16x@6b 708820833334ns (0.200969, -0.248819, 9.756614)
       GY data for lsm6dsv16x@6b 708587500003ns (0.004879, -0.007624, 0.005489)
       GY data for lsm6dsv16x@6b 708620833336ns (-0.006709, -0.001219, 0.012504)
       GY data for lsm6dsv16x@6b 708654166669ns (-0.001524, -0.004269, 0.006404)
       GY data for lsm6dsv16x@6b 708687500002ns (0.000304, -0.005184, 0.007319)
       GY data for lsm6dsv16x@6b 708720833335ns (0.001829, 0.000609, 0.003354)
       GY data for lsm6dsv16x@6b 708754166668ns (0.005489, -0.001524, 0.004574)
       GY data for lsm6dsv16x@6b 708787500001ns (0.052459, -0.010979, 0.041174)
       GY data for lsm6dsv16x@6b 708820833334ns (-0.021044, -0.008234, 0.089059)
       TP data for lsm6dsv16x@6b 708620833336ns 24.414062 °C
       TP data for lsm6dsv16x@6b 708687500002ns 24.382812 °C
       TP data for lsm6dsv16x@6b 708754166668ns 24.429687 °C
       TP data for lsm6dsv16x@6b 708820833334ns 24.421875 °C
       XL data for lsm6dsv16x@6b 708854166667ns (0.373229, 0.325379, 9.928874)
       XL data for lsm6dsv16x@6b 708887500000ns (0.086129, 0.119624, 10.986359)
       GY data for lsm6dsv16x@6b 708854166667ns (0.015249, 0.003049, 0.039954)
       GY data for lsm6dsv16x@6b 708887500000ns (0.025924, 0.071674, -0.101869)
       TP data for lsm6dsv16x@6b 708887500000ns 24.457031 °C

References
==========

.. target-notes::

.. _RTIO framework:
   https://docs.zephyrproject.org/latest/services/rtio/index.html

.. _x-nucleo-iks4a1:
   http://www.st.com/en/ecosystems/x-nucleo-iks4a1.html
