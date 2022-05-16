.. _i2c_api:

I2C
####

Overview
********

.. note::

   Zephyr recognizes the need to change the terms "master" and "slave"
   used in the current `I2C Specification <i2c-specification>`_.  This
   will be done when the conditions identified in
   :ref:`coding_guideline_inclusive_language` have been met.  Existing
   documentation, data structures, functions, and value symbols in code
   are likely to change at that point.

`I2C <i2c-specification>`_ (Inter-Integrated Circuit, pronounced "eye
squared see") is a commonly-used two-signal shared peripheral interface
bus.  Many system-on-chip solutions provide controllers that communicate
on an I2C bus.  Devices on the bus can operate in two roles: as a
"master" that initiates transactions and controls the clock, or as a
"slave" that responds to transaction commands.  A I2C controller on a
given SoC will generally support the master role, and some will also
support the slave mode.  Zephyr has API for both roles.

.. _i2c-master-api:

I2C Master API
==============

Zephyr's I2C master API is used when an I2C peripheral controls the bus,
in particularly the start and stop conditions and the clock.  This is
the most common mode, used to interact with I2C devices like sensors and
serial memory.

This API is supported in all in-tree I2C peripheral drivers and is
considered stable.

.. _i2c-slave-api:

I2C Slave API
================

Zephyr's I2C slave API is used when an I2C peripheral responds to
transactions initiated by a different controller on the bus.  It might
be used for a Zephyr application with transducer roles that are
controlled by another device such as a host processor.

This API is supported in very few in-tree I2C peripheral drivers.  The
API is considered experimental, as it is not compatible with the
capabilities of all I2C peripherals supported in master mode.


Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_I2C`

API Reference
*************

.. doxygengroup:: i2c_interface

.. _i2c-specification:
   https://www.nxp.com/docs/en/user-guide/UM10204.pdf
