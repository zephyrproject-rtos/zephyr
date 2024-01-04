.. _clock_control_api:

Clock Control
#############

Overview
********

The clock control API provides access to clocks in the system, including the
ability to turn them on and off.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_CLOCK_CONTROL`

Clock Setpoints
***************

The control control API includes the concept of "setpoints". Each setpoint
is intended to correspond to an independent clock tree state. For example,
one clock setpoint might power up the SOC's PLL, and clock multiple
peripherals from it. Another setpoint might power down the PLL, and clock
only peripherals in a low power domain.

Setpoints are intended to resolve the dependency problems that exist when
reconfiguring peripheral clocks, by requiring clock settings be defined
at the system level. Two standard clock setpoint IDs are defined:

.. table:: Standard clock setpoints
    :align: center

    +-------------+----------------------------------+-------------------------+
    | State       | Identifier                       | Purpose                 |
    +-------------+----------------------------------+-------------------------+
    | ``default`` | :c:macro:`CLOCK_SETPOINT_RUN`    | State of the clock tree |
    |             |                                  | when the device is      |
    |             |                                  | running                 |
    +-------------+----------------------------------+-------------------------+
    | ``idle``    | :c:macro:`CLOCK_SETPOINT_IDLE`   | State of the clock tree |
    |             |                                  | when the device is in   |
    |             |                                  | low power or idle mode  |
    +-------------+----------------------------------+-------------------------+

Clock control drivers can define additional setpoints, provided the definition
is greater than the value of :c:macro:`CLOCK_SETPOINT_PRIV_START`

API Reference
*************

.. doxygengroup:: clock_control_interface
