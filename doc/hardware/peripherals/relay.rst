.. _relay_api:

Relay
#####

The relay subsystem exposes a hardware-agnostic on/off view of a single relay,
regardless of whether the underlying part is switched by a GPIO line or driven
through a PWM channel. A backend driver implements the API and hides all
coil-drive details behind it.

Basic Operation
***************

Applications obtain a relay device through devicetree and drive it through the
functions in :zephyr_file:`include/zephyr/drivers/relay/relay.h`:

- :c:func:`relay_set_state` energizes or releases the coil, selecting between
  :c:enumerator:`RELAY_STATE_ON` and :c:enumerator:`RELAY_STATE_OFF`.
- :c:func:`relay_get_state` reads back the last requested state.

Backends
********

One devicetree-discoverable backend is provided:

- :dtcompatible:`zephyr,gpio-relay` for a relay switched by a single GPIO. The
  coil's active level comes from the flags in the ``gpios`` specifier.

Shell
*****

When :kconfig:option:`CONFIG_RELAY_SHELL` is enabled, the ``relay`` shell
command turns relays on and off, reads back their state, and lists the relay
devices in the system.

API Reference
*************

.. doxygengroup:: relay_interface
