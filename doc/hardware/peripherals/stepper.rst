.. _stepper_api:

Stepper API
###########

Overview
********

The Stepper API provides a set of functions for controlling and configuring stepper motors.
It supports a variety of operations, including enabling/disabling the controller, setting the
target position of the motor and thereby setting the motor in motion, setting/getting the actual
position of the motor and so on.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_STEPPER_INIT_PRIORITY`

API Reference
*************

.. doxygengroup:: stepper_interface
