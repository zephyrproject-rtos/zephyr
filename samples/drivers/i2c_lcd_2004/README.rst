.. _samples_i2c_lcd_2004:

LCD 2004 I2C Driver Sample
##########################

Overview
********
Display text on LCD through I2C. Text will be
displayed with 2 seconds delay between messages.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i2c_lcd_2004
   :host-os: unix
   :goals: run
   :compact:

Display output
==============

.. code-block:: console

    The Zephyr Project
    is a RTOS supporting
	multiple hardware
	architectures,

.. code-block:: console

	optimized for
	resource constrained
	devices, built with
	security in mind.
