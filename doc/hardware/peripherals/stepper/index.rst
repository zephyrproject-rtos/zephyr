.. _stepper_api:

Steppers
########

The stepper driver subsystem consists of two device driver APIs:

.. figure:: stepper_driver_subsystem_class_diagram.webp
   :align: center
   :alt: Stepper driver subsystem class diagram

Stepper-DRV API
***************

The stepper driver API provides a common interface for stepper drivers.

- Configure **micro-stepping resolution** using :c:func:`stepper_drv_set_micro_step_res`
  and :c:func:`stepper_drv_get_micro_step_res`.
- **Enable** the stepper driver using :c:func:`stepper_drv_enable`.
- **Disable** the stepper driver using :c:func:`stepper_drv_disable`.
- Set the **direction** of the stepper using :c:func:`stepper_drv_set_direction`.
- **Step** a single step using :c:func:`stepper_drv_step`.
- Register an **event callback** using :c:func:`stepper_drv_set_event_cb`.

Stepper API
***********

The stepper API provides a common interface for stepper controllers.

- Configure **reference position** in microsteps using :c:func:`stepper_set_reference_position`
  and :c:func:`stepper_get_actual_position`.
- Set **step interval** in nanoseconds between steps using :c:func:`stepper_set_microstep_interval`
- **Move by** +/- micro-steps also known as **relative movement** using :c:func:`stepper_move_by`.
- **Move to** a specific position also known as **absolute movement** using :c:func:`stepper_move_to`.
- Run continuously with a **constant step interval** in a specific direction until
  a stop is detected using :c:func:`stepper_run`.
- **Stop** the stepper using :c:func:`stepper_stop`.
- Check if the stepper is **moving** using :c:func:`stepper_is_moving`.
- Register an **event callback** using :c:func:`stepper_set_event_callback`.

.. _stepper-device-tree:

Device Tree
***********

In the context of stepper controllers  device tree provides the initial hardware
configuration for stepper drivers on a per device level. Each device must specify
a device tree binding in Zephyr, and ideally, a set of hardware configuration options
for things such as current settings, ramp parameters and furthermore. These can then
be used in a boards devicetree to configure a stepper driver to its initial state.

Driver Composition Scenarios
============================

Below are two typical scenarios:

.. toctree::
   :maxdepth: 1

   integrated_controller_driver.rst
   individual_controller_driver.rst

Discord
=======

Zephyr has a `stepper discord`_ channel for stepper related discussions, which
is open to all.

.. _stepper-api-reference:
.. _stepper-drv-api-reference:

Stepper API Test Suite
**********************

The stepper API test suite provides a set of tests that can be used to verify the functionality of
stepper drivers.

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/stepper/stepper_api
   :board: <board>
   :west-args: --extra-dtc-overlay <path/to/board.overlay>
   :goals: build flash

Sample Output
=============

Below is a snippet of the test output for the tmc50xx stepper driver. Since
:c:func:`stepper_set_microstep_interval` is not implemented by the driver the corresponding tests
have been skipped.

.. code-block:: console

   ===================================================================
   TESTSUITE stepper succeeded

   ------ TESTSUITE SUMMARY START ------

   SUITE PASS - 100.00% [stepper]: pass = 4, fail = 0, skip = 2, total = 6 duration = 0.069 seconds
    - PASS - [stepper.test_actual_position] duration = 0.016 seconds
    - PASS - [stepper.test_get_micro_step_res] duration = 0.013 seconds
    - SKIP - [stepper.test_set_micro_step_interval_invalid_zero] duration = 0.007 seconds
    - PASS - [stepper.test_set_micro_step_res_incorrect] duration = 0.010 seconds
    - PASS - [stepper.test_stop] duration = 0.016 seconds
    - SKIP - [stepper.test_target_position_w_fixed_step_interval] duration = 0.007 seconds

   ------ TESTSUITE SUMMARY END ------

   ===================================================================
   PROJECT EXECUTION SUCCESSFUL

API Reference
*************

A common set of functions which should be implemented by all stepper drivers.

.. doxygengroup:: stepper_drv_interface

A common set of functions which should be implemented by all stepper controllers.

.. doxygengroup:: stepper_interface

Stepper controller specific APIs
********************************

Trinamic
========

.. doxygengroup:: trinamic_stepper_interface

.. _stepper discord:
   https://discord.com/channels/720317445772017664/1278263869982375946
