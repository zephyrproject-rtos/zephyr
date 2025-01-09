.. _stepper_api:

Steppers
########

The stepper driver API provides a set of functions for controlling and configuring stepper drivers.

Configure Stepper Driver
========================

- Configure **micro-stepping resolution** using :c:func:`stepper_set_micro_step_res`
  and :c:func:`stepper_get_micro_step_res`.
- Configure **reference position** in microsteps using :c:func:`stepper_set_reference_position`
  and :c:func:`stepper_get_actual_position`.
- Set **step interval** in nanoseconds between steps using :c:func:`stepper_set_microstep_interval`
- **Enable** the stepper driver using :c:func:`stepper_enable`.

Control Stepper
===============

- **Move by** +/- micro-steps also known as **relative movement** using :c:func:`stepper_move_by`.
- **Move to** a specific position also known as **absolute movement** using :c:func:`stepper_move_to`.
- Run continuously with a **constant step interval** in a specific direction until
  a stop is detected using :c:func:`stepper_run`.
- Check if the stepper is **moving** using :c:func:`stepper_is_moving`.
- Register an **event callback** using :c:func:`stepper_set_event_callback`.

Device Tree
===========

In the context of stepper controllers  device tree provides the initial hardware
configuration for stepper drivers on a per device level. Each device must specify
a device tree binding in Zephyr, and ideally, a set of hardware configuration options
for things such as current settings, ramp parameters and furthermore. These can then
be used in a boards devicetree to configure a stepper driver to its initial state.

See examples in:

- :dtcompatible:`zephyr,gpio-stepper`
- :dtcompatible:`adi,tmc5041`

Discord
=======

Zephyr has a `stepper discord`_ channel for stepper related discussions, which
is open to all.

.. _stepper-api-reference:

API Reference
*************

A common set of functions which should be implemented by all stepper drivers.

.. doxygengroup:: stepper_interface

Stepper controller specific APIs
********************************

Trinamic
========

.. doxygengroup:: trinamic_stepper_interface

.. _stepper discord:
   https://discord.com/channels/720317445772017664/1278263869982375946
