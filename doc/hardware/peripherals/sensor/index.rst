.. _sensor:

Sensors
#######

The sensor driver API provides functionality to uniformly read, configure,
and setup event handling for devices that take real world measurements in
meaningful units.

Sensors range from very simple temperature-reading devices that must be polled
with a fixed scale to complex devices taking in readings from multitudes of
sensors and themselves producing new inferred sensor data such as step counts,
presence detection, orientation, and more.

Supporting this wide breadth of devices is a demanding task and the sensor API
attempts to provide a uniform interface to them.


.. _sensor-using:

Using Sensors
*************

Using sensors from an application there are some APIs and terms that are helpful
to understand. Sensors in Zephyr are composed of :ref:`sensor-channel`,
:ref:`sensor-attribute`, and :ref:`sensor-trigger`. Attributes and triggers may
be device or channel specific.

.. note::
   Getting samples from sensors using the sensor API today can be done in one of
   two ways. A stable and long-lived API :ref:`sensor-fetch-and-get`, or a newer
   but rapidly stabilizing API :ref:`sensor-read-and-decode`. It's expected that
   in the near future :ref:`sensor-fetch-and-get` will be deprecated in favor of
   :ref:`sensor-read-and-decode`. Triggers are handled entirely differently for
   :ref:`sensor-fetch-and-get` or :ref:`sensor-read-and-decode` and the
   differences are noted in each of those sections.

.. toctree::
   :maxdepth: 1

   attributes.rst
   channels.rst
   triggers.rst
   power_management.rst
   device_tree.rst
   fetch_and_get.rst
   read_and_decode.rst


.. _sensor-implementing:

Implementing Sensor Drivers
***************************

.. note::
   Implementing the driver side of the sensor API requires an understanding how
   the sensor APIs are used. Please read through :ref:`sensor-using` first!

Implementing Attributes
=======================

* SHOULD implement attribute setting in a blocking manner.
* SHOULD provide the ability to get and set channel scale if supported by the
  device.
* SHOULD provide the ability to get and set channel sampling rate if supported
  by the device.

Implementing Fetch and Get
==========================

* SHOULD implement :c:type:`sensor_sample_fetch_t` as a blocking call that
  stashes the specified channels (or all sensor channels) as driver instance
  data.
* SHOULD implement :c:type:`sensor_channel_get_t` without side effects
  manipulating driver state returning stashed sensor readings.
* SHOULD implement :c:type:`sensor_trigger_set_t` storing the address of the
  :c:struct:`sensor_trigger` rather than copying the contents. This is so
  :c:macro:`CONTAINER_OF` may be used for trigger callback context.

Implementing Read and Decode
============================

* MUST implement :c:type:`sensor_submit_t` as a non-blocking call.
* SHOULD implement :c:type:`sensor_submit_t` using :ref:`rtio` to do non-blocking bus transfers if possible.
* MAY implement :c:type:`sensor_submit_t` using a work queue if
  :ref:`rtio` is unsupported by the bus.
* SHOULD implement :c:type:`sensor_submit_t` checking the :c:struct:`rtio_sqe`
  is of type :c:enum:`RTIO_SQE_RX` (read request).
* SHOULD implement :c:type:`sensor_submit_t` checking all requested channels
  supported or respond with an error if not.
* SHOULD implement :c:type:`sensor_submit_t` checking the provided buffer
  is large enough for the requested channels.
* SHOULD implement :c:type:`sensor_submit_t` in a way that directly reads into
  the provided buffer avoiding copying of any kind, with few exceptions.
* MUST implement :c:struct:`sensor_decoder_api` with pure stateless functions. All state needed to convert the raw sensor readings into
  fixed point SI united values must be in the provided buffer.
* MUST implement :c:type:`sensor_get_decoder_t` returning the
  :c:struct:`sensor_decoder_api` for that device type.

.. _sensor-api-reference:

API Reference
***************

.. doxygengroup:: sensor_interface
.. doxygengroup:: sensor_emulator_backend
