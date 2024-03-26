.. _pm-system:

System Power Management
#######################

The kernel enters the idle state when it has nothing to schedule.
Enabling :kconfig:option:`CONFIG_PM` allows the kernel to call upon the
power management subsystem to put an idle system into one of the supported power states.

The PM subsystem selects the appropriate power state to transition to
depending on the following factors:

* The amount of time prescribed by the kernel to suspend upon invocation of the power manager.
* The chosen power management policy.

It is the application's responsibility to set up a wake-up event.
A wake-up event will typically be an interrupt triggered by an SoC peripheral module.
Examples include a SysTick, RTC, counter, or GPIO.
Keep in mind that depending on the SoC and the power mode in question,
not all peripherals may be active, and therefore
some wake-up sources may not be usable in all power modes.

The following diagram describes system power management:

.. image:: images/system-pm.svg
   :align: center
   :alt: System power management


Power States
============

The power management subsystem defines a set of states described by the
power consumption and context retention associated with each of them.

The high level description of each state is as follows, each having more or less
the cumulative effects of the previous:

* ACTIVE: System is fully powered and active.
* RUNTIME_IDLE: Cores enter deepest idle state waiting for interrupts,
  all other devices are unaffected.
* SUSPEND_TO_IDLE: May put some peripherals into low power states.
* STANDBY: Peripherals are put into low power states, and non boot cores are powered off.
* SUSPEND_TO_RAM: Powers off as much of the system as possible.
  Execution state and device state is held in ram.
* SUSPEND_TO_DISK: saves to a disk instead of ram.
* SOFT_OFF: essentially nothing is preserved, wake-up will be like a system reboot.

These power states are defined by :c:enum:`pm_state`. In general, lower power states
(higher index in the enum) will offer greater power savings and have higher wake latencies.

Power Management Policies
=========================

The power management subsystem supports the following power management policies:

* Residency based
* Application defined

The policy manager is the component of the power management subsystem responsible
for deciding which power state the system should transition to.
The policy manager can only choose between states that have been defined for the platform.
Other constraints placed upon the decision may include locks disallowing certain power states,
or various kinds of minimum and maximum latency values, depending on the policy.

More details on the states definition can be found in the
:dtcompatible:`zephyr,power-state` binding documentation.

Residency
---------

Under the residency policy, the system will enter the power state which offers the highest
power savings, with the constraint that the sum of the minimum residency value (see
:dtcompatible:`zephyr,power-state`) and the latency to exit the mode must be
less than or equal to the system idle time duration scheduled by the kernel.

Thus the core logic can be summarized with the following expression:

.. code-block:: c

   if (time_to_next_scheduled_event >= (state.min_residency_us + state.exit_latency)) {
      return state
   }

Application
-----------

The application defines the power management policy by implementing the
:c:func:`pm_policy_next_state` function. In this policy, the application is free
to decide which power state the system should transition to based on the
remaining time until the next scheduled timeout.

An example of an application that defines its own policy can be found in
:zephyr_file:`tests/subsys/pm/power_mgmt/`.

Policy and Power States
------------------------

The power management subsystem allows different Zephyr components and
applications to configure the policy manager to block the system from transitioning
into certain power states. This can be used by devices when executing tasks in
background to prevent the system from going to a specific state where it would
lose context. See :c:func:`pm_policy_state_lock_get`.

Examples
========

Some helpful examples showing different power management features:

* :zephyr_file:`samples/boards/stm32/power_mgmt/blinky/`
* :zephyr_file:`samples/boards/esp32/deep_sleep/`
* :zephyr_file:`samples/subsys/pm/device_pm/`
* :zephyr_file:`tests/subsys/pm/power_mgmt/`
* :zephyr_file:`tests/subsys/pm/power_mgmt_soc/`
* :zephyr_file:`tests/subsys/pm/power_states_api/`
