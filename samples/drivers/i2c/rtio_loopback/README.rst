.. zephyr:code-sample:: i2c-rtio-loopback
   :name: I2C RTIO loopback
   :relevant-api: rtio i2c_interface

   Perform I2C transfers between I2C controller and custom I2C target using RTIO.

Overview
********

This sample demonstrates how to perform I2C transfers, synchronously and async
using RTIO. It uses up to two I2C controllers, acting as I2C controller and
target.

Requirements
************

This sample requires either:

* Two I2C controllers, one supporting the I2C controller role, one supporting the
  I2C peripheral role, both connected to the same I2C bus.
* An I2C controller supporting both I2C controller and peripheral role
  simultaneously.

.. note::

   Remember to set up the I2C bus, connecting SCL and SDA pull-up resistors, and
   connecting the relevant I2C controllers to the bus physically.

Board support
*************

Any board which meets the requirements must use an overlay to specify which
I2C controller will act as the controller, and which as the peripheral, note
that this could be the same controller. This is done using the devicetree
aliases ``i2c-controller`` and ``i2c-controller-target`` respectively:

.. code-block:: devicetree

   / {
           aliases {
                   i2c-controller = &i2c1;
                   i2c-controller-target = &i2c2;
           };
   };

If necessary, add any board specific configs to the board specific overlay:

.. code-block:: cfg

   CONFIG_I2C_TARGET_BUFFER_MODE=y
