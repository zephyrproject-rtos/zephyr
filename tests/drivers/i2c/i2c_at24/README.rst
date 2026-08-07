.. SPDX-License-Identifier: Apache-2.0

.. _i2c_at24_tests:

I2C AT24 EEPROM Test
####################

Overview
========

This test validates I2C controller driver APIs using an AT24-compatible
EEPROM as a test fixture. It exercises raw I2C, RTIO, and optional
PM code paths.

.. important::

   This is NOT an EEPROM driver test. The EEPROM subsystem is disabled
   (``CONFIG_EEPROM=n``) to avoid driver contention.

Hardware Setup
==============

* Board with I2C controller
* AT24-compatible EEPROM on the I2C bus

Devicetree Requirements
=======================

The EEPROM node must specify:

* ``compatible = "atmel,at24"``
* ``reg`` - I2C slave address
* ``size``, ``pagesize``, ``address-width``, ``timeout``

Use the ``eeprom-0`` alias to select the test target. See
``boards/at24.overlay`` for an example.

Building
========

Raw I2C (default)::

   west build -b <board> tests/drivers/i2c/i2c_at24 -- \
     -DEXTRA_DTC_OVERLAY_FILE=boards/at24.overlay

PM runtime variant::

   west build -b <board> tests/drivers/i2c/i2c_at24 -T drivers.i2c.at24.pm

RTIO variant::

   west build -b <board> tests/drivers/i2c/i2c_at24 -T drivers.i2c.at24.rtio

PM + RTIO combined::

   west build -b <board> tests/drivers/i2c/i2c_at24 -T drivers.i2c.at24.pm.rtio
