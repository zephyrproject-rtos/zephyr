.. _stepper_api:

Steppers
########

The stepper driver subsystem consists of two device driver APIs:

Stepper API
***********

The stepper driver API provides a common interface for stepper drivers.

- Configure **micro-stepping resolution** using :c:func:`stepper_set_micro_step_res`
  and :c:func:`stepper_get_micro_step_res`.
- **Enable** the stepper driver using :c:func:`stepper_enable`.
- **Disable** the stepper driver using :c:func:`stepper_disable`.
- Register an **event callback** using :c:func:`stepper_set_event_cb`.

Stepper Motion Controller API
*****************************

The stepper motion controller API provides a common interface for stepper motion controllers.

- Configure **reference position** in microsteps using :c:func:`stepper_ctrl_set_reference_position`
  and :c:func:`stepper_ctrl_get_actual_position`.
- Set **step interval** in nanoseconds between steps using :c:func:`stepper_ctrl_set_microstep_interval`
- **Move by** +/- micro-steps also known as **relative movement** using :c:func:`stepper_ctrl_move_by`.
- **Move to** a specific position also known as **absolute movement** using :c:func:`stepper_ctrl_move_to`.
- Run continuously with a **constant step interval** in a specific direction until
  a stop is detected using :c:func:`stepper_ctrl_run`.
- **Stop** the stepper using :c:func:`stepper_ctrl_stop`.
- Check if the stepper is **moving** using :c:func:`stepper_ctrl_is_moving`.
- Register an **event callback** using :c:func:`stepper_ctrl_set_event_cb`.

.. _stepper-device-tree:

Device Tree
***********

In the context of stepper motion controllers, devicetree provides the initial hardware
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

Stepper Motion Controller API Test Suite
****************************************

The stepper motion controller API test suite provides a set of tests that can be used to verify
the functionality of stepper motion controllers.

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/stepper/stepper_ctrl
   :board: <board>
   :west-args: --extra-dtc-overlay <path/to/board.overlay>
   :goals: build flash

Sample Output
=============

Below is a snippet of the test output for the h-bridge-stepper-ctrl.

.. code-block:: console

   ===================================================================
   TESTSUITE stepper succeeded

   ------ TESTSUITE SUMMARY START ------

   SUITE PASS - 100.00% [stepper_ctrl]: pass = 10, fail = 0, skip = 0, total = 10 duration = 6.869 seconds
    - PASS - [stepper_ctrl.test_actual_position] duration = 0.001 seconds
    - PASS - [stepper_ctrl.test_move_by_negative_step_count] duration = 2.207 seconds
    - PASS - [stepper_ctrl.test_move_by_positive_step_count] duration = 2.202 seconds
    - PASS - [stepper_ctrl.test_move_to_negative_step_count] duration = 1.106 seconds
    - PASS - [stepper_ctrl.test_move_to_positive_step_count] duration = 1.102 seconds
    - PASS - [stepper_ctrl.test_move_zero_steps] duration = 0.006 seconds
    - PASS - [stepper_ctrl.test_run_negative_direction] duration = 0.115 seconds
    - PASS - [stepper_ctrl.test_run_positive_direction] duration = 0.124 seconds
    - PASS - [stepper_ctrl.test_set_micro_step_interval_invalid_zero] duration = 0.002 seconds
    - PASS - [stepper_ctrl.test_stop] duration = 0.004 seconds

   ------ TESTSUITE SUMMARY END ------

   ===================================================================
   PROJECT EXECUTION SUCCESSFUL

API Reference
*************

.. _stepper-driver-api-reference:

A common set of functions which should be implemented by all stepper drivers.

.. doxygengroup:: stepper_hw_driver_interface

.. _stepper-ctrl-api-reference:

A common set of functions which should be implemented by all stepper motion controllers.

.. doxygengroup:: stepper_ctrl_interface

Stepper motion controller specific APIs
***************************************

Trinamic
========

.. doxygengroup:: trinamic_stepper_interface

.. _stepper discord:
   https://discord.com/channels/720317445772017664/1278263869982375946
