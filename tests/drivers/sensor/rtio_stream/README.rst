.. Copyright (c) 2026 Analog Devices, Inc.
.. SPDX-License-Identifier: Apache-2.0

Sensor RTIO stream tests
########################

Exercises a streaming sensor through the sensor read and decode API only
(:c:func:`sensor_stream`, :c:func:`sensor_read`, the sensor decoder and RTIO):

#. ``test_1_stream_rate_and_data``: the stream delivers at the sensor's output
   data rate, every frame decodes, and the mean reads about 1 g at rest.
#. ``test_2_restart_after_idle``: a stream restarted after sitting stopped
   while the sensor kept sampling delivers again.
#. ``test_3_restart_cycles``: repeated stop/restart cycles, each stopped for a
   different time, all deliver.
#. ``test_4_one_shot_read``: a one-shot read decodes to one frame of about
   1 g (the sensor must be at rest).
#. ``test_5_one_shot_after_stream``: the same, after a stream has run and
   stopped.
#. ``test_6_restart_after_pool_exhausted``: after a consumer stopped releasing
   captures and let the RTIO mempool run dry, a new stream delivers. It runs
   last, as a driver that fails it streams no more until reset.

The device under test is the ``accel0`` alias. What it must deliver comes from
the ``zephyr,user`` node, so another streaming accelerometer needs only an
overlay and a Kconfig fragment enabling its stream support:

.. code-block:: devicetree

   / {
           zephyr,user {
                   stream-rate-hz = <400>;          /* required: the sensor's ODR */
                   rate-tolerance-percent = <5>;    /* default 5 */
                   one-g-tolerance-mg = <150>;      /* default 200 */
                   capture-timeout-ms = <1000>;     /* default 1000 */
                   stop-quiet-ms = <300>;           /* default 300 */
                   idle-ms = <1000>;                /* default 1000 */
                   restart-cycles = <10>;           /* default 10 */
           };
   };

Set the sensor's ODR in the same overlay so that it matches
``stream-rate-hz``.

Supported setups
****************

* ADXL345 on the :ref:`pmod_acl` shield (``adxl345.conf``,
  ``adxl345.overlay``, 400 Hz).
* ADXL372 on the :ref:`eval_adxl372_ardz` shield (``adxl372.conf``,
  ``adxl372.overlay``, 1600 Hz).

Building and running
********************

.. code-block:: console

   west build -p -b apard32690/max32690/m4 tests/drivers/sensor/rtio_stream -- \
       -DSHIELD=pmod_acl -DEXTRA_CONF_FILE=adxl345.conf \
       -DEXTRA_DTC_OVERLAY_FILE=adxl345.overlay
   west flash

With twister, the board attached and the shield fitted:

.. code-block:: console

   west twister -T tests/drivers/sensor/rtio_stream -p apard32690/max32690/m4 \
       --device-testing --device-serial <port> --fixture pmod_acl
