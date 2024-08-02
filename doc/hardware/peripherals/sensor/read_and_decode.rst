.. _sensor-read-and-decode:

Read and Decode
###############

The quickly stabilizing experimental APIs for reading sensor data are:

* :c:func:`sensor_read`
* :c:func:`sensor_read_async_mempool`
* :c:func:`sensor_get_decoder`
* :c:func:`sensor_decode`


Benefits over :ref:`sensor-fetch-and-get`
*********************************************************

These APIs allow for a wider usage of sensors, sensor types, and data flows with
sensors. These are the future looking APIs in Zephyr and solve many issues
that have been run into with :ref:`sensor-fetch-and-get`.

:c:func:`sensor_read` and similar functions acquire sensor encoded data into
a buffer provided by the caller. Decode (:c:func:`sensor_decode`) then
decodes the sensor specific encoded data into fixed point :c:type:`q31_t` values
as vectors per channel. This allows further processing using fixed point DSP
functions that work on vectors of data to be done (e.g. low-pass filters, FFT,
fusion, etc).

Reading is by default asynchronous in its implementation and takes advantage of
:ref:`rtio` to enable chaining asynchronous requests, or starting requests
against many sensors simultaneously from a single call context.

This enables incredibly useful code flows when working with sensors such as:

* Obtaining the raw sensor data, decoding never, later, or on a separate
  processor (e.g. a phone).
* Starting a read for sensors directly from an interrupt handler. No dedicated
  thread needed saving precious stack space. No work queue needed introducing
  variable latency. Starting a read for multiple sensors simultaneously from a
  single call context (interrupt/thread/work queue).
* Requesting multiple reads to the same device for Ping-Pong (double buffering)
  setups.
* Creating entire pipelines of data flow from sensors allowing for software
  defined virtual sensors (:ref:`sensing`) all from a single thread with DAG
  process ordering.
* Potentially pre-programming DMAs to trigger on GPIO events, leaving the CPU
  entirely out of the loop in handling sensor events like FIFO watermarks.

Additionally, other shortcomings of :ref:`sensor-fetch-and-get` related to memory
and trigger handling are solved.

* Triggers result in enqueued events, not callbacks.
* Triggers can be setup to automatically fetch data. Potentially
  enabling pre-programmed DMA transfers on GPIO interrupts.
* Far less likely triggers are missed due to long held interrupt masks from
  callbacks and context swapping.
* Sensor FIFOs supported by wiring up FIFO triggers to read data into
  mempool allocated buffers.
* All sensor processing can be done in user mode (memory protected) threads.
* Multiple sensor channels of the same type are better supported.

.. note::
   For `Read and Decode`_ benefits to be fully realized requires
   :ref:`rtio` compliant communication access to the sensor. Typically this means
   an :ref:`rtio` enabled bus driver for SPI or I2C.

Polling Read
************

Polling reads with `Read and Decode`_ can be accomplished by instantiating a
polling I/O device (akin to a file descriptor) for the sensor with the desired
channels to poll. Requesting either blocking or non-blocking reads, then
optionally decoding the data into fixed point values.

Polling a temperature sensor and printing its readout is likely the simplest
sample to show how this all works.

.. literalinclude:: temp_polling.c
   :language: c

Polling Read with Multiple Sensors
**********************************

One of the benefits of Read and Decode is the ability to concurrently read many
sensors with many channels in one thread. Effectively read requests are started
asynchronously for all sensors and their channels. When each read completes we
then decode the sensor data. Examples speak loudly and so a sample showing how
this might work with multiple temperature sensors with multiple temperature
channels:

.. literalinclude:: multiple_temp_polling.c
   :language: c

Streaming
*********

Handling triggers with `Read and Decode`_ works by setting up a stream I/O device
configuration. A stream specifies the set of triggers to capture and if data
should be captured with the event.



.. literalinclude:: accel_stream.c
   :language: c
