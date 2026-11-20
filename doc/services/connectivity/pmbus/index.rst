.. _pmbus:

PMBus
#####

Overview
********

PMBus (Power Management Bus) is a command protocol for power conversion and
power management devices. It is defined by the System Management Interface
Forum in the `PMBus Specifications`_ and uses SMBus as its transport.

The PMBus core is a library used by drivers of PMBus devices. It does not
define a device class. A driver keeps a :c:struct:`pmbus_dev` in its data and
calls :c:func:`pmbus_init` from its init function. The core then provides:

* Command access with Read/Write Byte, Read/Write Word and Send Byte, on top of
  the :ref:`SMBus API <smbus_api>`.
* Page handling: the PAGE command is sent only when the requested page differs
  from the one last selected, under a lock that keeps the page and the command
  that follows together.
* Detection of supported commands with :c:func:`pmbus_cmd_supported`, which
  checks for a NACK or an invalid command reported in STATUS_CML.
* Fault report at initialization: :c:func:`pmbus_init` logs the STATUS_WORD
  of each page that reports a fault, before any CLEAR_FAULTS it or the
  detection of commands sends drops the faults latched before.
* Telemetry read with :c:func:`pmbus_read_value`, which converts READ_VOUT
  with the ULINEAR16 exponent from VOUT_MODE and the other READ commands from
  LINEAR11 to integer values in milli units.

Packet Error Checking is a setting of the SMBus controller and applies to all
devices on the bus. It is enabled with :c:func:`smbus_configure`, and
:c:func:`pmbus_init` checks it against the CAPABILITY command of the device.
On a board without an SMBus controller driver, the
:dtcompatible:`zephyr,smbus-i2c` controller provides SMBus on top of an I2C
controller.

Generic sensor driver
*********************

Devices that follow the specification without device specific handling can
use the :dtcompatible:`zephyr,pmbus` sensor driver. At initialization it
detects which of READ_VOUT, READ_IOUT, READ_POUT and READ_TEMPERATURE_1 the
device supports with :c:func:`pmbus_cmd_supported`, and exposes them on page 0
as the voltage, current, power and ambient temperature channels. See the
:zephyr:code-sample:`pmbus` sample.

Limitations
***********

* DIRECT, VID and IEEE 754 half precision formats are not supported.
* Block commands, SMBALERT#, Group Command, Zone and extended commands are not
  supported.

Configuration
*************

* :kconfig:option:`CONFIG_PMBUS`
* :kconfig:option:`CONFIG_PMBUS_MAX_PAGES`
* :kconfig:option:`CONFIG_SENSOR_PMBUS`

API Reference
*************

.. doxygengroup:: pmbus

.. _PMBus Specifications:
   https://pmbus.org/specification-archives/
