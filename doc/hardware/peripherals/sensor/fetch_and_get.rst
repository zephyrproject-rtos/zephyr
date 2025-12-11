.. _sensor-fetch-and-get:

Fetch and Get
#############

The stable and long existing APIs for reading sensor data and handling triggers
are:

* :c:func:`sensor_sample_fetch`
* :c:func:`sensor_sample_fetch_chan`
* :c:func:`sensor_channel_get`
* :c:func:`sensor_trigger_set`

These functions work together. The fetch APIs block the calling context which
must be a thread until the requested :c:enum:`sensor_channel` (or all channels)
has been obtained and stored into the driver instance's private data.

The channel data most recently fetched can then be obtained as a
:c:struct:`sensor_value` by calling :c:func:`sensor_channel_get` for each channel
type.

.. warning::
   It should be noted that calling fetch and get from multiple contexts without
   a locking mechanism is undefined and most sensor drivers do not attempt to
   internally provide exclusive access to the device during or between these
   calls.

Polling
*******

Using fetch and get sensor can be read in a polling manner from software threads.


.. literalinclude:: ../../../../samples/sensor/magn_polling/src/main.c
   :language: c

Triggers
********

Triggers in the stable API require enabling triggers with a device
specific Kconfig. The device specific Kconfig typically allows selecting the
context the trigger runs. The application then needs to register a callback with
a function signature matching :c:type:`sensor_trigger_handler_t` using
:c:func:`sensor_trigger_set` for the specific triggers (events) to listen for.

.. note::
   Triggers may not be set from user mode threads, and the callback is not
   run in a user mode context.

There are typically two options provided for each driver where to run trigger
handlers. Either the trigger handler is run using the system work queue thread
(:ref:`workqueues_v2`) or a dedicated thread. A great example can be found in
the BMI160 driver which has Kconfig options for selecting a trigger mode.
See :kconfig:option:`CONFIG_BMI160_TRIGGER_NONE`,
:kconfig:option:`CONFIG_BMI160_TRIGGER_GLOBAL_THREAD` (work queue),
:kconfig:option:`CONFIG_BMI160_TRIGGER_OWN_THREAD` (dedicated thread).

Some notable attributes of using a driver dedicated thread vs the system work
queue.

* Driver dedicated threads have dedicated stack (RAM) which only gets used for
  that single trigger handler function.
* Driver dedicated threads *do* get their own priority typically which lets you
  prioritize trigger handling among other threads.
* Driver dedicated threads will not have head of line blocking if the driver
  needs time to handle the trigger.

.. note::
   In all cases it's very likely there will be variable delays from the actual
   interrupt to your callback function being run. In the work queue
   (GLOBAL_THREAD) case the work queue itself can be the source of variable
   latency!

.. literalinclude:: tap_count.c
   :language: c
