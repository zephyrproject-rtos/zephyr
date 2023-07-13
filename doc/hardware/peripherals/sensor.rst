.. _sensor_api:

Sensors
#######

The sensor subsystem exposes an API to uniformly access sensor devices.
Common operations are: reading data and executing code when specific
conditions are met.

Basic Operation
***************

Channels
========

Fundamentally, a channel is a quantity that a sensor device can measure.

Sensors can have multiple channels, either to represent different axes of
the same physical property (e.g. acceleration); or because they can measure
different properties altogether (ambient temperature, pressure and
humidity).  Complex sensors cover both cases, so a single device can expose
three acceleration channels and a temperature one.

It is imperative that all sensors that support a given channel express
results in the same unit of measurement. Consult the
:ref:`sensor_api_reference` for all supported channels, along with their
description and units of measurement:

Values
======

Sensor stable APIs return results as :c:struct:`sensor_value`.  This
representation avoids use of floating point values as they may not be
supported on certain setups.

A newer experimental (may change) API that can interpret raw sensor data is
available in parallel. This new API exposes raw encoded sensor data to the
application and provides a separate decoder to convert the data to a Q31 format
which is compatible with the Zephyr :ref:`zdsp_api`. The values represented are
in the range of (-1.0, 1.0) and require a shift operation in order to scale
them to their SI unit values. See :ref:`Async Read` for more information.

Fetching Values
===============

Getting a reading from a sensor requires two operations.  First, an
application instructs the driver to fetch a sample of all its channels.
Then, individual channels may be read.  In the case of channels with
multiple axes, they can be read in a single operation by supplying
the corresponding :literal:`_XYZ` channel type and a buffer of 3
:c:struct:`sensor_value` objects.  This approach ensures consistency
of channels between reads and efficiency of communication by issuing a
single transaction on the underlying bus.

Below is an example illustrating the usage of the BME280 sensor, which
measures ambient temperature and atmospheric pressure.  Note that
:c:func:`sensor_sample_fetch` is only called once, as it reads and
compensates data for both channels.

.. literalinclude:: ../../../samples/sensor/bme280/src/main.c
   :language: c
   :lines: 12-
   :linenos:

.. _Async Read:

Async Read
==========

To enable the async APIs, use :kconfig:option:`CONFIG_SENSOR_ASYNC_API`.

Reading the sensors leverages the :ref:`rtio_api` subsystem. Applications
gain control of the data processing thread and even memory management. In order
to get started with reading the sensors, an IODev must be created via the
:c:macro:`SENSOR_DT_READ_IODEV`. Next, an RTIO context must be created. It is
strongly suggested that this context is created with a memory pool via
:c:macro:`RTIO_DEFINE_WITH_MEMPOOL`.

.. code-block:: C

   #include <zephyr/device.h>
   #include <zephyr/drivers/sensor.h>
   #include <zephyr/rtio/rtio.h>

   static const struct device *lid_accel = DEVICE_DT_GET(DT_ALIAS(lid_accel));
   SENSOR_DT_READ_IODEV(lid_accel_iodev, DT_ALIAS(lid_accel), SENSOR_CHAN_ACCEL_XYZ);

   RTIO_DEFINE_WITH_MEMPOOL(sensors_rtio,
                            4,  /* submission queue size */
                            4,  /* completion queue size */
                            16, /* number of memory blocks */
                            32, /* size of each memory block */
                            4   /* memory alignment */
                            );

To trigger a read, the application simply needs to call :c:func:`sensor_read`
and pass the relevant IODev and RTIO context. Getting the result is done like
any other RTIO operation, by waiting on a completion queue event (CQE). In
order to help reduce some boilerplate code, the helper function
:c:func:`sensor_processing_with_callback` is provided. When called, the
function will block until a CQE becomes available from the provided RTIO
context. The appropriate buffers are extracted and the callback is called.
Once the callback is done, the memory is reclaimed by the memorypool. This
looks like:

.. code-block:: C

   static void sensor_processing_callback(int result, uint8_t *buf,
                                          uint32_t buf_len, void *userdata) {
     // Process the data...
   }

   static void sensor_processing_thread(void *, void *, void *) {
     while (true) {
       sensor_processing_with_callback(&sensors_rtio, sensor_processing_callback);
     }
   }
   K_THREAD_DEFINE(sensor_processing_tid, 1024, sensor_processing_thread,
                   NULL, NULL, NULL, 0, 0, 0);

.. note::
   Helper functions to create custom length IODev nodes and ones that don't
   have static bindings will be added soon.

Processing the Data
===================

Once data collection completes and the processing callback was called,
processing the data is done via the :c:struct:`sensor_decoder_api`. The API
provides a means for applications to control *when* to process the data and how
many resources to dedicate to the processing. The API is entirely self
contained and requires no system calls (even when
:kconfig:option:`CONFIG_USERSPACE` is enabled).

.. code-block:: C

   static struct sensor_decoder_api *lid_accel_decoder = SENSOR_DECODER_DT_GET(DT_ALIAS(lid_accel));

   static void sensor_processing_callback(int result, uint8_t *buf,
                                          uint32_t buf_len, void *userdata) {
     uint64_t timestamp;
     sensor_frame_iterator_t fit = {0};
     sensor_channel_iterator_t cit = {0};
     enum sensor_channel channels[3];
     q31_t values[3];
     int8_t shift[3];

     lid_accel_decoder->get_timestamp(buf, &timestamp);
     lid_accel_decoder->decode(buf, &fit, &cit, channels, values, 3);

     /* Values are now in q31_t format, we're going to convert them to micro-units */

     /* First, we need to know by how much to shift the values */
     lid_accel_decoder->get_shift(buf, channels[0], &shift[0]);
     lid_accel_decoder->get_shift(buf, channels[1], &shift[1]);
     lid_accel_decoder->get_shift(buf, channels[2], &shift[2]);

     /* Shift the values to get the SI units */
     int64_t scaled_values[] = {
       (int64_t)values[0] << shift[0],
       (int64_t)values[1] << shift[1],
       (int64_t)values[2] << shift[2],
     };

     /*
      * FIELD_GET(GENMASK64(63, 31), scaled_values[]) - will give the integer value
      * FIELD_GET(GENMASK64(30, 0), scaled_values[]) / INT32_MAX - is the decimal value
      */
   }

Configuration and Attributes
****************************

Setting the communication bus and address is considered the most basic
configuration for sensor devices.  This setting is done at compile time, via
the configuration menu.  If the sensor supports interrupts, the interrupt
lines and triggering parameters described below are also configured at
compile time.

Alongside these communication parameters, sensor chips typically expose
multiple parameters that control the accuracy and frequency of measurement.
In compliance with Zephyr's design goals, most of these values are
statically configured at compile time.

However, certain parameters could require runtime configuration, for
example, threshold values for interrupts.  These values are configured via
attributes.  The example in the following section showcases a sensor with an
interrupt line that is triggered when the temperature crosses a threshold.
The threshold is configured at runtime using an attribute.

Triggers
********

:dfn:`Triggers` in Zephyr refer to the interrupt lines of the sensor chips.
Many sensor chips support one or more triggers. Some examples of triggers
include: new data is ready for reading, a channel value has crossed a
threshold, or the device has sensed motion.

To configure a trigger, an application needs to supply a
:c:struct:`sensor_trigger` and a handler function.  The structure contains the
trigger type and the channel on which the trigger must be configured.

Because most sensors are connected via SPI or I2C buses, it is not possible
to communicate with them from the interrupt execution context.  The
execution of the trigger handler is deferred to a thread, so that data
fetching operations are possible.  A driver can spawn its own thread to fetch
data, thus ensuring minimum latency. Alternatively, multiple sensor drivers
can share a system-wide thread. The shared thread approach increases the
latency of handling interrupts but uses less memory. You can configure which
approach to follow for each driver. Most drivers can entirely disable
triggers resulting in a smaller footprint.

The following example contains a trigger fired whenever temperature crosses
the 26 degree Celsius threshold. It also samples the temperature every
second. A real application would ideally disable periodic sampling in the
interest of saving power. Since the application has direct access to the
kernel config symbols, no trigger is registered when triggering was disabled
by the driver's configuration.

.. literalinclude:: ../../../samples/sensor/mcp9808/src/main.c
   :language: c
   :lines: 12-
   :linenos:

.. _sensor_api_reference:

API Reference
**************

.. doxygengroup:: sensor_interface
.. doxygengroup:: sensor_emulator_backend
