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


Per-Controller Transfer Timeout
*******************************

Each I2C controller driver uses an internal timeout to bound how long a
transfer may block the calling thread.  By default drivers rely on a
compile-time constant (e.g. :kconfig:option:`CONFIG_I2C_STM32_TRANSFER_TIMEOUT_MSEC`
for STM32 controllers, 100 ms for ITE controllers).

Board designers can override this on a per-controller basis by adding the
``transfer-timeout-ms`` property to the controller's DTS node:

.. code-block:: devicetree

   &i2c1 {
       transfer-timeout-ms = <50>;
   };

When the property is absent the driver's existing default is used unchanged,
so boards that do not set it see no behavioural difference.

This property is defined in the base ``i2c-controller.yaml`` binding and
is therefore available on all I2C controller nodes regardless of SoC vendor.

.. note::

   Setting a very short timeout on a bus that carries slow devices (e.g.
   EEPROMs performing internal write cycles) will cause spurious
   ``-ETIMEDOUT`` errors.  Conversely, setting a very long timeout delays
   error detection on a hung bus.  Choose a value that is comfortably
   longer than the slowest expected transfer on the bus but short enough
   to bound worst-case thread blocking time.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_I2C`

API Reference
*************

.. doxygengroup:: i2c_interface

.. _i2c-specification:
   https://www.nxp.com/docs/en/user-guide/UM10204.pdf

.. _I2C: i2c-specification_
