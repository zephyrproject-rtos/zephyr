.. _smbus_api:

System Management Bus (SMBus)
#############################

.. contents::
    :local:
    :depth: 2

Overview
********

System Management Bus (SMBus) is derived from  I2C for communication
with devices on the motherboard. A system may use SMBus to communicate
with the peripherals on the motherboard without using dedicated control
lines. With SMBus peripheral can provide various manufacturer information,
report errors, accept control parameters, etc.

Devices on the bus can operate in three roles: as a "controller" that
initiates transactions and controls the clock, a "peripheral" that
responds to transaction commands, or a "host", which is a specialized
controller, that provides the main interface to the system's CPU.
Zephyr has API for "controller" role.

In SMBus peripheral devices can initiate communication with Controller
with two methods:

* **Host Notify protocol**: Peripheral device that supports Host Notify
  protocol behaive as a Controller to perform the notification. It writes
  three-bytes message to a special address "SMBus Host (0x08)" with own
  address and two bytes of relevant data.
* **SMBALERT# signal**: Peripheral device uses special signal SMBALERT# to
  to request attention from the Controller. Controller needs to read one byte
  from the special "SMBus Alert Response Address (ARA) (0x0c)". The peripheral
  device responds with data byte containing own address.

Currently, the API is based on `SMBus Specification`_ version 2.0

.. note::
   See :ref:`coding_guideline_inclusive_language` for information about
   the terminology used in this API.

.. _smbus-controller-api:

SMBus Controller API
********************

Zephyr's SMBus controller API is used when an SMBus device controls the bus,
in particularly the start and stop conditions and the clock.  This is
the most common mode, used to interact with SMbus peripherals.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_SMBUS`

API Reference
*************

.. doxygengroup:: smbus_interface

.. _SMBus Specification: http://smbus.org/specs/smbus20.pdf
