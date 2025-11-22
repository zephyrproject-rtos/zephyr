.. _actuator_api:

Actuator
########

Overview
********

An actuator is a physical device, which actuates to affect a physical system. Actuation is, within
this device class, defined as a specific and directional applied force, torque or displacement
within an absolute range.

Examples of actuators matching the actuator device class:

* Servo motor rotating to a specific angle.
* Linear actuator moving to a specific extension.
* Motor rotating at a specific angular velocity.
* Motor applying a specific torque to an axel.

Examples of actuators which do not match the actuator device class:

* Stepper motor moving in incremental steps. See :ref:`stepper_api` for stepper motor support.
* Solenoid either actuating or not actuating.

Related configuration options:

* :kconfig:option:`CONFIG_ACTUATOR`

Devicetree bindings
*******************

Actuator bindings must include the ``actuator-device.yaml`` binding, which includes the
``base.yaml`` binding and the required ``actuator-invert`` property.

.. code-block:: yaml

   include: actuator-device.yaml

Device driver design
********************

The following rules must be adhered to by actuator device drivers:

* If the ``actuator-invert`` devicetree property is set for an instance of an actuator device, the
  direction of actuation must be inverted.

* No actuation shall occur if no setpoint has been set, or if the actuator device is suspended by
  device PM.

* The last set setpoint must be retained if an actuator device is suspended by device PM.

Actuator shell
**************

The actuator shell provides the ``actuator`` command with the ``set_setpoint`` subcommand for the
:ref:`shell <shell_api>` module.

The ``set_setpoint`` command takes two arguments:

* The actuator device to set the setpoint of.
* The setpoint to set in 1/1000 steps (permille) from -1 to 1 (-1000 to 1000).

Related configuration options:

* :kconfig:option:`CONFIG_SHELL`
* :kconfig:option:`CONFIG_ACTUATOR_SHELL`

Actuator API Reference
**********************

.. doxygengroup:: actuator_interface
