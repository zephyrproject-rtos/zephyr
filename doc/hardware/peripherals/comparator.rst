.. _comparator_api:

Comparator
##########

Overview
********

An analog comparator compares the voltages of two analog signals connected to its negative and
positive inputs. If the voltage at the positive input is higher than the negative input, the
comparator's output will be high, otherwise, it will be low.

Comparators can typically set a trigger which triggers on output changes. This trigger can
either invoke a callback, or its status can be polled.

Related configuration options:

* :kconfig:option:`CONFIG_COMPARATOR`

Configuration
*************

Embedded comparators can typically be configured at runtime. When enabled, an initial
configuration must be provided using the devicetree. At runtime, comparators can have their
configuration updated using device driver specific APIs. The configuration will be applied
when the comparator is resumed.

Power management
****************

Comparators are enabled using power management. When resumed, the comparator will actively
compare its inputs, producing an output and detecting edges. When suspended, the comparator
will be inactive.

Comparator shell
****************

The comparator shell provides the ``comp`` command with a set of subcommands for the
:ref:`shell <shell_api>` module.

The ``comp`` shell command provides the following subcommands:

* ``get_output`` See :c:func:`comparator_get_output`
* ``set_trigger`` See :c:func:`comparator_set_trigger`
* ``await_trigger`` Awaits trigger using the following flow:
  * Set trigger callback using :c:func:`comparator_set_trigger_callback`
  * Await callback or time out after default or optionally provided timeout
  * Clear trigger callback using :c:func:`comparator_set_trigger_callback`
* ``trigger_is_pending`` See :c:func:`comparator_trigger_is_pending`

Related configuration options:

* :kconfig:option:`CONFIG_SHELL`
* :kconfig:option:`CONFIG_COMPARATOR_SHELL`
* :kconfig:option:`CONFIG_COMPARATOR_SHELL_AWAIT_TRIGGER_DEFAULT_TIMEOUT`
* :kconfig:option:`CONFIG_COMPARATOR_SHELL_AWAIT_TRIGGER_MAX_TIMEOUT`

.. note::
   The power management shell can optionally be enabled alongside the comparator shell.

   Related configuration options:

   * :kconfig:option:`CONFIG_PM_DEVICE`
   * :kconfig:option:`CONFIG_PM_DEVICE_SHELL`

API Reference
*************

.. doxygengroup:: comparator_interface
