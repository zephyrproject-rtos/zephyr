.. _i2c_api:

Inter-Integrated Circuit (I2C) Bus
##################################

Overview
********

.. note::

   The terminology used in Zephyr I2C APIs follows that of the
   `NXP I2C Bus Specification Rev 7.0 <i2c-specification_>`_. These changed
   from previous revisions as of its release October 1, 2021.

`I2C`_ (Inter-Integrated Circuit, pronounced "eye
squared see") is a commonly-used two-signal shared peripheral interface
bus.  Many system-on-chip solutions provide controllers that communicate
on an I2C bus.  Devices on the bus can operate in two roles: as a
"controller" that initiates transactions and controls the clock, or as a
"target" that responds to transaction commands.  A I2C controller on a
given SoC will generally support the controller role, and some will also
support the target mode.  Zephyr has API for both roles.

.. _i2c-controller-api:

I2C Controller API
==================

Zephyr's I2C controller API is used when an I2C peripheral controls the bus,
particularly the start and stop conditions and the clock.  This is
the most common mode, used to interact with I2C devices like sensors and
serial memory.

This API is supported in all in-tree I2C peripheral drivers and is
considered stable.

.. _i2c-target-api:

I2C Target API
==============

Zephyr's I2C target API is used when an I2C peripheral responds to
transactions initiated by a different controller on the bus.  It might
be used for a Zephyr application with transducer roles that are
controlled by another device such as a host processor.

This API is supported in very few in-tree I2C peripheral drivers.  The
API is considered experimental, as it is not compatible with the
capabilities of all I2C peripherals supported in controller mode.


Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_I2C`
* :kconfig:option:`CONFIG_I2C_TRANSFER_TIMEOUT_MS`

Transfer Timeout
================

The I2C subsystem provides two complementary mechanisms to control how long a
driver waits for a transfer to complete before returning ``-ETIMEDOUT``.

Application-wide default (Kconfig)
-----------------------------------

:kconfig:option:`CONFIG_I2C_TRANSFER_TIMEOUT_MS` sets the default timeout in
milliseconds for all I2C controllers whose drivers opt in by selecting
``I2C_TRANSFER_TIMEOUT_SUPPORTED``.  A value of ``0`` means wait forever
(``K_FOREVER``).  Drivers that do not permit an infinite timeout (e.g. those
that program the value directly into a hardware register) use
:c:macro:`BUILD_ASSERT_INVALID_I2C_TRANSFER_TIMEOUT` to enforce a non-zero
value at build time.

Per-controller DT override
---------------------------

Individual controllers can override the application-wide default through the
``zephyr,transfer-timeout-ms`` devicetree property defined in
``dts/bindings/i2c/i2c-controller.yaml``.  When the property is absent the
driver falls back to :kconfig:option:`CONFIG_I2C_TRANSFER_TIMEOUT_MS`.

Example board overlay::

    &i2c1 {
        /* Fast sensor bus - fail quickly on a hung device */
        zephyr,transfer-timeout-ms = <50>;
    };

    &i2c2 {
        /* EEPROM bus - allow for long internal write cycles */
        zephyr,transfer-timeout-ms = <2000>;
    };

Drivers that support per-instance timeouts use
:c:macro:`I2C_DT_INST_TRANSFER_TIMEOUT` (or :c:macro:`I2C_DT_INST_TRANSFER_TIMEOUT_MS`
for raw integer use) which resolve the timeout with the following priority:

1. ``zephyr,transfer-timeout-ms`` DT property on the controller node
2. :kconfig:option:`CONFIG_I2C_TRANSFER_TIMEOUT_MS`
3. ``K_FOREVER`` when the resolved value is ``0``

API Reference
*************

.. doxygengroup:: i2c_interface

.. _i2c-specification:
   https://www.nxp.com/docs/en/user-guide/UM10204.pdf

.. _I2C: i2c-specification_
