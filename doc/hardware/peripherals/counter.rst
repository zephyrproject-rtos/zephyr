.. _counter_api:

Counter
#######

Overview
********

The Counter subsystem provides a generic API for hardware and software
counter/timer devices. It abstracts common functionality such as counting,
alarms, and timing events, allowing applications to operate independently
of specific hardware implementations.

Counters are typically used for:

- Time measurement
- Generating periodic or one-shot events
- Scheduling callbacks via alarms
- Tracking elapsed ticks or cycles

Features
********

The Counter API supports the following key features:

- **Up/Down Counting**
  Devices may support incrementing or decrementing counters.

- **Top Value Configuration**
  Ability to configure the counter's maximum (top) value and wrapping behavior.

- **Alarms / Callbacks**
  - Single-shot alarms (applications must set the new alarm after expiry).
  - Callback execution when a specified tick value is reached

- **Absolute and Relative Alarms**
  - Absolute alarms trigger at a specific counter value
  - Relative alarms trigger after a specified number of ticks

- **Capture**
  Records the counter value when an external or internal event occurs,
  enabling precise timestamping of signals.

- **Guard Period**
  The guard period defines a minimum time margin required between the
  current counter value and a requested alarm value. If an alarm is set
  within this margin, it is considered *late to set*. When the
  :c:macro:`COUNTER_GUARD_PERIOD_LATE_TO_SET` flag is used, the driver
  detects this condition and reports it instead of silently missing the alarm.

- **Multiple Channels**
  Some devices support multiple independent alarm channels.

- **Start/Stop Control**
  Ability to start and stop the counter dynamically.

- **Tick Conversion Utilities**
  Conversion between ticks and time units (e.g., microseconds, nanoseconds).

Basic Operations
****************

Typical workflow when using counter alarm:

1. Initialize and retrieve the device.
2. Configure the top value and guard value (optional).
3. Set an alarm callback.
4. Start the counter.
5. Handle callbacks when alarms are triggered.

Typical workflow when using capture:

1. Initialize and retrieve the device.
2. Configure capture channel and its callback.
3. Enable capture for the channel.
4. Trigger capture event and handle the callback.

Configuration Options
*********************

* :kconfig:option:`CONFIG_COUNTER`
* :kconfig:option:`CONFIG_COUNTER_CAPTURE`

API Reference
*************

.. doxygengroup:: counter_interface
