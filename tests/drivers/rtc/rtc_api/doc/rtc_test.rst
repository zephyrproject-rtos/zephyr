.. _rtc_api_test:

:Author: Bjarki Arge Andreasen

RTC Test suite
###############

Test RTC API implementation for RTC devices.

Overview
********

This suite is design to be portable between boards. It uses the alias
``rtc`` to designate the RTC device to test.

This test suite tests the following:

* Setting and getting the time.
* RTC Time incrementing correctly.
* Alarms if supported by hardware, with and without callback enabled
* Calibration if supported by hardware.

The calibration test tests a range of values which are printed to the
console to be manually compared. The user must review the set and
gotten values to ensure they are valid.

By default, only the mandatory Setting and getting time is enabled
for testing. To test the optional alarms, update event callback
and clock calibration, these must be enabled by selecting
``CONFIG_RTC_ALARM``, ``CONFIG_RTC_UPDATE`` and
``CONFIG_RTC_CALIBRATION``.
