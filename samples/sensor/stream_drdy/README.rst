.. zephyr:code-sample:: stream_drdy
   :name: Generic device sample streaming using Data Ready trigger
   :relevant-api: sensor_interface

   Get accelerometer data frames from a sensor using SENSOR_TRIG_DATA_READY.

Overview
********

This sample application demonstrates how to stream accel data using the
:ref:`RTIO framework <rtio>` based :ref:`Read and Decode method <sensor-read-and-decode>`.

The streaming is started using the sensor_stream() API and it is self-sustained by the
SENSOR_TRIG_DATA_READY trigger.

Currently the sample gets/prints data only for the accel sensor channel:

        - SENSOR_CHAN_ACCEL_XYZ

Building and Running
********************

This sample supports up to 10 streaming devices. Each device needs
to be aliased as ``streamN`` where ``N`` goes from ``0`` to ``9``. For example:

.. code-block:: devicetree

  / {
      aliases {
                   stream0 = &lsm6dsv16x_6b_x_nucleo_iks4a1;
              };
      };

Example devicetree overlays and configurations are already available for sensortile_box_pro,
nucleo_f401re and nucleo_h503rb in the boards directory:

- :zephyr_file:`samples/sensor/stream_drdy/boards/sensortile_box_pro.overlay`

  DT overlay file for the sensortile_box_pro board.

- :zephyr_file:`samples/sensor/stream_drdy/boards/sensortile_box_pro.conf`

  Configuration file for the sensortile_box_pro board.

- :zephyr_file:`samples/sensor/stream_drdy/boards/nucleo_f401re.overlay`

  DT overlay file for the nucleo_f401re board.

- :zephyr_file:`samples/sensor/stream_drdy/boards/nucleo_f401re.conf`

  Configuration file for the nucleo_f401re board.

- :zephyr_file:`samples/sensor/stream_drdy/boards/nucleo_h503rb.overlay`

  DT overlay file for the nucleo_h503rb board.

- :zephyr_file:`samples/sensor/stream_drdy/boards/nucleo_h503rb.conf`

  Configuration file for the nucleo_h503rb board.

For example, build and run sample for sensortile_box_pro with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/stream_drdy
   :board: sensortile_box_pro
   :goals: build flash
   :compact:

Sample Output
=============

The following example output is for a lsm6dsv16x IMU device with accelerometer sensor.
The board used is a sensortile_box_pro.

.. code-block:: console

XL data for lsm6dsv16x@0 7320515312ns (-0.387584, 0.224894, 9.766184)
XL data for lsm6dsv16x@0 7321538600ns (-0.363659, 0.282314, 9.948014)
XL data for lsm6dsv16x@0 7322561362ns (-0.301454, 0.172259, 9.775754)
XL data for lsm6dsv16x@0 7323584881ns (-0.210539, 0.153119, 9.857099)
XL data for lsm6dsv16x@0 7324608368ns (-0.287099, 0.167474, 9.852314)
XL data for lsm6dsv16x@0 7325631281ns (-0.306239, 0.181829, 9.847529)
XL data for lsm6dsv16x@0 7326654425ns (-0.272744, 0.167474, 9.842744)
XL data for lsm6dsv16x@0 7327677993ns (-0.296669, 0.224894, 9.981509)
XL data for lsm6dsv16x@0 7328701506ns (-0.282314, 0.210539, 9.828389)
XL data for lsm6dsv16x@0 7329724306ns (-0.244034, 0.153119, 9.866669)
XL data for lsm6dsv16x@0 7330747556ns (-0.234464, 0.119624, 9.780539)
XL data for lsm6dsv16x@0 7331771000ns (-0.239249, 0.148334, 9.933659)
XL data for lsm6dsv16x@0 7332794575ns (-0.220109, 0.119624, 9.833174)
XL data for lsm6dsv16x@0 7333817437ns (-0.205754, 0.119624, 9.823604)
XL data for lsm6dsv16x@0 7334840643ns (-0.205754, 0.148334, 9.866669)
XL data for lsm6dsv16x@0 7335864162ns (-0.186614, 0.129194, 9.861884)
XL data for lsm6dsv16x@0 7336887593ns (-0.196184, 0.110054, 9.804464)
XL data for lsm6dsv16x@0 7337910356ns (-0.181829, 0.133979, 9.938444)
XL data for lsm6dsv16x@0 7338933650ns (-0.215324, 0.081344, 9.536504)
XL data for lsm6dsv16x@0 7339957075ns (-0.157904, 0.119624, 9.995864)
XL data for lsm6dsv16x@0 7340980675ns (-0.205754, 0.110054, 9.809249)
XL data for lsm6dsv16x@0 7342003487ns (-0.177044, 0.143549, 9.971939)
XL data for lsm6dsv16x@0 7343026593ns (-0.172259, 0.100484, 9.794894)
XL data for lsm6dsv16x@0 7344050168ns (-0.177044, 0.124409, 9.881024)
XL data for lsm6dsv16x@0 7345073643ns (-0.191399, 0.124409, 9.986294)
XL data for lsm6dsv16x@0 7346096587ns (-0.191399, 0.105269, 9.790109)

References
==========

.. target-notes::

.. _RTIO framework:
   https://docs.zephyrproject.org/latest/services/rtio/index.html

.. _x-nucleo-iks4a1:
   http://www.st.com/en/ecosystems/x-nucleo-iks4a1.html
