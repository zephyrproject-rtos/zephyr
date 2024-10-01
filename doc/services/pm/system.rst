.. _pm-system:

System Power Management
#######################

Introduction
************

The kernel enters the idle state when it has nothing to schedule.
Enabling :kconfig:option:`CONFIG_PM` allows the kernel to call upon the
power management subsystem to put an idle system into one of the supported power states.
The kernel requests an amount of time it would like to suspend, then the PM subsystem decides
the appropriate power state to transition to based on the configured power management policy.

It is the application's responsibility to set up a wake-up event.
A wake-up event will typically be an interrupt triggered by an SoC peripheral module.
Examples include a SysTick, RTC, counter, or GPIO.
Keep in mind that depending on the SoC and the power mode in question,
not all peripherals may be active, and therefore
some wake-up sources may not be usable in all power modes.

The following diagram describes system power management:

.. graphviz::
   :caption: System power management

   digraph G {
       compound=true
       node [height=1.2 style=rounded]

       lock [label="Lock interruptions"]
       config_pm [label="CONFIG_PM" shape=diamond style="rounded,dashed"]
       forced_state [label="state forced ?", shape=diamond style="rounded,dashed"]
       config_system_managed_device_pm [label="CONFIG_PM_DEVICE" shape=diamond style="rounded,dashed"]
       config_system_managed_device_pm2 [label="CONFIG_PM_DEVICE" shape=diamond style="rounded,dashed"]
       pm_policy [label="Check policy manager\nfor a power state "]
       pm_suspend_devices [label="Suspend\ndevices"]
       pm_resume_devices [label="Resume\ndevices"]
       pm_state_set [label="Change power state\n(SoC API)" style="rounded,bold"]
       pm_system_resume [label="Finish wake-up\n(SoC API)\n(restore interruptions)" style="rounded,bold"]
       k_cpu_idle [label="k_cpu_idle()"]

       subgraph cluster_0 {
              style=invisible;
              lock -> config_pm
       }

       subgraph cluster_1 {
                style=dashed
                label = "pm_system_suspend()"

                forced_state -> config_system_managed_device_pm [label="yes"]
                forced_state -> pm_policy [label="no"]
                pm_policy -> config_system_managed_device_pm
                config_system_managed_device_pm -> pm_state_set:e [label="no"]
                config_system_managed_device_pm -> pm_suspend_devices [label="yes"]
                pm_suspend_devices -> pm_state_set
                pm_state_set -> config_system_managed_device_pm2
                config_system_managed_device_pm2 -> pm_resume_devices [label="yes"]
                config_system_managed_device_pm2 -> pm_system_resume [label="no"]
                pm_resume_devices -> pm_system_resume
        }

        {rankdir=LR k_cpu_idle; forced_state}
        pm_policy -> k_cpu_idle [label="PM_STATE_ACTIVE\n(no power state meet requirements)"]
        config_pm -> k_cpu_idle [label="no"]
        config_pm -> forced_state [label="yes" lhead="cluster_1"]
        pm_system_resume:e -> lock:e [constraint=false lhed="cluster_0"]
   }


Power States
============

The power management subsystem defines a set of states described by the
power consumption and context retention associated with each of them.

The set of power states is defined by :c:enum:`pm_state`. In general, lower power states
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

.. _pm-policy-power-states:

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

* :zephyr_file:`samples/boards/st/power_mgmt/blinky/`
* :zephyr_file:`samples/boards/espressif/deep_sleep/`
* :zephyr_file:`samples/subsys/pm/device_pm/`
* :zephyr_file:`tests/subsys/pm/power_mgmt/`
* :zephyr_file:`tests/subsys/pm/power_mgmt_soc/`
* :zephyr_file:`tests/subsys/pm/power_states_api/`
