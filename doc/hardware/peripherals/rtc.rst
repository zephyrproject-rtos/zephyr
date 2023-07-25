.. _rtc_api:

Real-Time Clock (RTC)
#####################

Overview
********

.. list-table:: **Glossary**
    :widths: 30 80
    :header-rows: 1

    * - Word
      - Definition
    * - Real-time clock
      - Low power device tracking time using broken-down time
    * - Real-time counter
      - Low power counter which can be used to track time
    * - RTC
      - Acronym for real-time clock

An RTC is a low power device which tracks time using broken-down time.
It should not be confused with low-power counters which sometimes share
the same name, acronym, or both.

RTCs are usually optimized for low energy consumption and are usually
kept running even when the system is in a low power state.

RTCs usually contain one or more alarms which can be configured to
trigger at a given time. These alarms are commonly used to wake up the
system from a low power state.

History of RTCs in Zephyr
*************************

RTCs have been supported before this API was created, using the
:ref:`counter_api` API. The unix timestamp was used to convert
between broken-down time and the unix timestamp within the RTC
drivers, which internally used the broken-down time representation.

The disadvantages of this approach were that hardware counters could
not be set to a specific count, requiring all RTCs to use device
specific APIs to set the time, converting from unix time to
broken-down time, unnecessarily in some cases, and some common
features missing, like input clock calibration and the update
callback.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_RTC`
* :kconfig:option:`CONFIG_RTC_ALARM`
* :kconfig:option:`CONFIG_RTC_UPDATE`
* :kconfig:option:`CONFIG_RTC_CALIBRATION`

API Reference
*************

.. doxygengroup:: rtc_interface

RTC device driver test suite
****************************

The test suite validates the behavior of the RTC device driver. It
is designed to be portable between boards. It uses the device tree
alias ``rtc`` to designate the RTC device to test.

This test suite tests the following:

* Setting and getting the time.
* RTC Time incrementing correctly.
* Alarms if supported by hardware, with and without callback enabled
* Calibration if supported by hardware.

The calibration test tests a range of values which are printed to the
console to be manually compared. The user must review the set and
gotten values to ensure they are valid.

By default, only the mandatory setting and getting of time is enabled
for testing. To test the optional alarms, update event callback
and clock calibration, these must be enabled by selecting
:kconfig:option:`CONFIG_RTC_ALARM`, :kconfig:option:`CONFIG_RTC_UPDATE`
and :kconfig:option:`CONFIG_RTC_CALIBRATION`.

The following examples build the test suite for the ``native_posix``
board. To build the test suite for a different board, replace the
``native_posix`` board with your board.

To build the test application with the default configuration, testing
only the mandatory features, the following command can be used for
reference:

.. zephyr-app-commands::
   :tool: west
   :host-os: unix
   :board: native_posix
   :zephyr-app: tests/drivers/rtc/rtc_api
   :goals: build

To build the test with additional RTC features enabled, use menuconfig
to enable the additional features by updating the configuration. The
following command can be used for reference:

.. zephyr-app-commands::
   :tool: west
   :host-os: unix
   :board: native_posix
   :zephyr-app: tests/drivers/rtc/rtc_api
   :goals: menuconfig

Then build the test application using the following command:

.. zephyr-app-commands::
   :tool: west
   :host-os: unix
   :board: native_posix
   :zephyr-app: tests/drivers/rtc/rtc_api
   :maybe-skip-config:
   :goals: build

To run the test suite, flash and run the application on your board, the output will
be printed to the console.

.. note::

    The tests take up to 30 seconds each if they are testing real hardware.
