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

Sensor devices return results as :c:struct:`sensor_value`.  This
representation avoids use of floating point values as they may not be
supported on certain setups.

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

The example assumes that the returned values have type :c:struct:`sensor_value`,
which is the case for BME280.  A real application
supporting multiple sensors should inspect the :c:data:`type` field of
the :c:data:`temp` and :c:data:`press` values and use the other fields
of the structure accordingly.

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

Because most sensors are connected via SPI or I2C busses, it is not possible
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
   :project: Zephyr
