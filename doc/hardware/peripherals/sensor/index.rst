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

Adding your ``sensor.yaml``
===========================

Sensor YAML files can be added to enhance your driver writing experience. Each
sensor can introduce a ``sensor.yaml`` file which follows the schema described
in `pw_sensor`_. The file will automatically be picked up by the build system
and entries will be created for your driver. See the example at
``tests/drivers/sensor/generator/`` for the macro usage, you can do things
like:

* Get the number of attributes, channels, and triggers supported by a specific
  compatible string.
* Get the number of instances of a specific channel of a sensor.
* Check if a driver supports setting a specific attribute/channel pair.
* Automatically index your drivers into the Zephyr documentation

The primary use case for these is to verify that a DT entry's compatible value
supports specific requirements. For example:

* Assume we're building a lid angle calculation which requires 2
  accelerometers. These accelerometers will be specified in the devicetree
  using the chosen nodes so we can access them via ``DT_CHOSEN(lid_accel)``
  and ``DT_CHOSEN(base_accel)``.
* We can now run a static assert like
  ``BUILD_ASSERT(SENSOR_SPEC_CHAN_INST_COUNT(DT_PROP(DT_CHOSEN(lid_accel), compatible), accel_xyz) > 0);``

Additional features are in development and will allow drivers to:

* Generate boilerplate code for the sensor API such as attribute set/get,
  submit, etc.
* Generate common config/data struct definitions.
* Abstract the bus API so you can read/write registers without worrying about
  i2c/spi/etc.
* Optimize memory allocation based to YAML metadata. Using the static data
  available in the ``sensor.yaml`` file, we will be able to allocate the right
  size buffer for streaming and one-shot reading APIs as well as client side
  buffers.

Documenting your driver
-----------------------

In your ``sensor.yaml`` include a field:

.. code-block:: yaml

   extras:
      doc-link: "my-document-link"

Then include a document in your driver that uses the same anchor name:

.. code-block:: rst

   .. _my-document-link:

   My driver
   =========

   ...

Adding sensors out of tree
--------------------------

Zephyr supports adding sensors from modules. There are 3 CMake variables that
should be updated to make that happen:

- ``ZEPHYR_EXTRA_SENSOR_YAML_FILES``: this is a cmake list containing full
  paths to sensor YAML files that are not in the Zephyr tree.
- ``ZEPHYR_EXTRA_SENSOR_INCLUDE_PATHS``: this is a cmake list containing full
  paths to directories which contains custom attributes, channels, triggers, or
  units. These paths are directories which the ``zephyr_sensor_library`` cmake
  function will use to find definition files which are included in your sensor
  YAML file. It's similar to header include paths.
- ``ZEPHYR_EXTRA_SENSOR_DEFINITION_FILES``: this is a cmake list containing
  full paths to files containing custom attributes, channels, triggers, or
  units. The file list is used to determine when the sensor library needs to be
  rebuilt.

.. _sensor-api-reference:

API Reference
***************

.. doxygengroup:: sensor_interface
.. doxygengroup:: sensor_emulator_backend

.. _`pw_sensor`: https://pigweed.dev/pw_sensor/py/
