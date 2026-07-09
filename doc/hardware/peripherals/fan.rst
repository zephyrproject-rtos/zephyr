.. _fan_api:

Fan
###

The fan subsystem exposes an API to control cooling fans and similar variable-speed blowers
uniformly, regardless of the underlying hardware.

Basic Operation
***************

Applications obtain a fan device through devicetree and drive it through the functions in
:zephyr_file:`include/zephyr/drivers/fan.h`:

- :c:func:`fan_set_speed` sets the fan speed as a percentage of full speed, from 0 (stopped) to
  :c:macro:`FAN_SPEED_MAX` (full speed).
- :c:func:`fan_get_speed` returns the last speed requested through :c:func:`fan_set_speed`. This
  is the commanded value, not a measured rotation rate.
- :c:func:`fan_get_rpm` reads the fan's rotation rate in revolutions per minute from a tachometer
  or equivalent feedback signal. Backends without a tachometer report :c:macro:`ENOSYS`.

Backends
********

One devicetree-discoverable backend is provided:

- :dtcompatible:`fan-pwm` for fans whose speed is controlled by a PWM channel. The PWM duty cycle
  sets the speed, from 0% (stopped) to 100% (full speed). An optional tachometer input allows the
  driver to report the measured rotation rate through :c:func:`fan_get_rpm`.

API Reference
*************

.. doxygengroup:: fan_interface
