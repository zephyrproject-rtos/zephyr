.. zephyr:code-sample:: lvgl-accelerometer-chart
   :name: LVGL line chart with accelerometer data
   :relevant-api: display_interface sensor_interface

   Display acceleration data on a real-time chart using LVGL.

Overview
********

A sample application that demonstrates how to use LVGL and the :ref:`sensor` to
display acceleration data on a line chart that is updated in real time.

This sample creates a line chart with three series, one for each axis of the accelerometer. An LVGL
timer fetches the latest acceleration data from the sensor every 20 ms (default value) and updates
the chart. The update period is configurable, see
:ref:`lvgl_accelerometer_chart_building_and_running` below.

Requirements
************

* A board with a display.
* A sensor that provides acceleration data (:c:enumerator:`SENSOR_CHAN_ACCEL_XYZ`) and available
  under the ``&accel0`` Devicetree alias.

.. note::

   A Devicetree overlay declaring an emulated BMI160 accelerometer is provided for the
   ``native_sim*`` board variants, making it possible to run this sample on
   your local machine.

.. _lvgl_accelerometer_chart_building_and_running:

Building and Running
********************

The maximum number of points to display for each series and the sampling rate of the
accelerometer can be configured using the :kconfig:option:`CONFIG_SAMPLE_CHART_POINTS_PER_SERIES`
and :kconfig:option:`CONFIG_SAMPLE_ACCEL_SAMPLING_RATE` Kconfig options, respectively.

The default sampling rate is 50 Hz, and the default maximum number of points per series is 50.

The demo can be built as follows (note how in this particular case the sampling rate is set to a
custom value of 20 Hz):

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/accelerometer_chart
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONFIG_SAMPLE_ACCEL_SAMPLING_RATE=20
   :goals: run
   :compact:
