.. _reset_api:

Reset Controller
################

Overview
********

Reset controllers are units that control the reset signals to multiple
peripherals. The reset controller API allows peripheral drivers to request
control over their reset input signals, including the ability to assert,
deassert and toggle those signals. Also, the reset status of the reset input
signal can be checked.

Mainly, the line_assert and line_deassert API functions are optional because
in most cases we want to toggle the reset signals.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_RESET`

API Reference
*************

.. doxygengroup:: reset_controller_interface
