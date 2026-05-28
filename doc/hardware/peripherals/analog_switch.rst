.. _analog_switch_api:

Analog Switch
#############

Overview
********

The analog switch API provides a generic interface for controlling analog
signal switches. Supported topologies include single-pole single-throw (SPST),
single-pole double-throw (SPDT) and MEMS switches.

Each device exposes one or more independently controllable channels. Channels
are addressed by a zero-based index and can be manipulated individually with
:c:func:`analog_switch_set` / :c:func:`analog_switch_get`, or all at once
with :c:func:`analog_switch_set_all` / :c:func:`analog_switch_get_all`.

Channel Model
*************

A channel's state is a small unsigned integer whose meaning is part-specific
and documented in the corresponding devicetree binding:

- **SPST** devices use 0 for *open* (no connection) and 1 for *closed*
  (connection active).
- **SPDT** devices use 0 to select *path B* and 1 to select *path A*.

The bitmask helpers :c:func:`analog_switch_set_all` and
:c:func:`analog_switch_get_all` pack every channel state into a single
``uint32_t``, where bit *n* represents channel *n*. Bits beyond the device's
channel count are ignored on write and cleared on read.

:c:func:`analog_switch_reset` returns all channels to the device's default
state, which is typically *all open* for SPST and MEMS parts.

Enable / Disable
****************

Some parts expose a hardware enable line. When such a device is disabled via
:c:func:`analog_switch_disable`, all channels are forced open regardless of
the last requested state. Channel state set while disabled takes effect on the
next :c:func:`analog_switch_enable` call.

Parts without a hardware enable line return ``-ENOTSUP`` from these functions.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_ANALOG_SWITCH`

API Reference
*************

.. doxygengroup:: analog_switch_interface
