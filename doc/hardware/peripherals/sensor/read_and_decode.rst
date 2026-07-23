.. _sensor-read-and-decode:

Read and Decode
###############

The quickly stabilizing APIs for reading sensor data are:

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

.. _sensor-fixed-point:

Fixed-point values
******************

Decoded sensor data is returned as :c:type:`q31_t` fixed-point values. A
``q31_t`` is a 32-bit signed integer container paired with a *shift* that places
the binary point, letting drivers report fractional readings without using
floating point.

Q notation and the shift
========================

Fixed-point numbers are written using the notation :math:`Q_{m.n}`, where:

:math:`m`
   number of integer bits

:math:`n`
   number of fractional bits

Zephyr's sensor subsystem uses a 32-bit signed container and specifies only
:math:`m` as the shift:

.. math::

   Q_{m.(31-m)} \quad\quad 0 \leq m \leq 31

with one sign bit and :math:`31 - m` fractional bits.

The shift value is a tradeoff between  precision for range, increases in shift value
increase range of real numbers the value can occupy at the cost of the step or value
per lsb being larger and hence the loss of precision

For example, a shift of 10 (10 integer bits and 21 fractional bits) encoded value spans:

.. math::

   -2^{10} \leq \text{value} < 2^{10} \quad\quad (-1024 \leq \text{value} < 1024)

while the value per LSB (the smallest encodable step) is:

.. math::

   \text{value per LSB} = \frac{1}{2^{31 - m}}

which is :math:`1 / 2^{21}` at a shift of 10.

Scaling raw sensor samples
==========================

Sensors are usually read as ``int16`` or ``int24`` raw counts. The resolution is
then set by the full-scale range (:math:`FS`) the sensor maps onto its
integer-only representation of width :math:`N` bits:

.. math::

   \text{value per LSB} = \frac{FS}{2^{N - 1}}

For example, an accelerometer with a +/-8 g full-scale range read as a signed
16-bit sample resolves:

.. math::

   \frac{8}{2^{15}} = \frac{8}{32768} = \frac{1}{4096} \text{ g per LSB}

A driver converts the raw count to an SI value using this scale (and offset, if any),
then encodes that value as ``q31_t`` at the channel's shift.

Conversion helpers
==================

The following helpers convert between an integer SI value (in micro, milli, or
centi units) and the ``q31_t`` representation at a given shift:

* :c:func:`sensor_q31_from_micro`, :c:func:`sensor_q31_from_milli`,
  :c:func:`sensor_q31_from_centi`
* :c:func:`sensor_q31_to_micro`, :c:func:`sensor_q31_to_milli`,
  :c:func:`sensor_q31_to_centi`

The :zephyr_file:`tests/drivers/sensor/utils` test suite exercises these helpers
with ``int16`` samples for an IMU and a temperature sensor as worked examples.

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
