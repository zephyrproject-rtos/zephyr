.. _pm-system:

System Power Management
#######################

The kernel enters the idle state when it has nothing to schedule. If enabled via
the :kconfig:option:`CONFIG_PM` Kconfig option, the Power Management
Subsystem can put an idle system in one of the supported power states, based
on the selected power management policy and the duration of the idle time
allotted by the kernel.

It is an application responsibility to set up a wake up event. A wake up event
will typically be an interrupt triggered by one of the SoC peripheral modules
such as a SysTick, RTC, counter, or GPIO. Depending on the power mode entered,
only some SoC peripheral modules may be active and can be used as a wake up
source.

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

Some handful examples using different power management features:

* :zephyr_file:`samples/boards/stm32/power_mgmt/blinky/`
* :zephyr_file:`samples/boards/esp32/deep_sleep/`
* :zephyr_file:`samples/subsys/pm/device_pm/`
* :zephyr_file:`tests/subsys/pm/power_mgmt/`
* :zephyr_file:`tests/subsys/pm/power_mgmt_soc/`
* :zephyr_file:`tests/subsys/pm/power_states_api/`

Power States
============

The power management subsystem contains a set of states based on
power consumption and context retention.

The list of available power states is defined by :c:enum:`pm_state`. In
general power states with higher indexes will offer greater power savings and
have higher wake latencies.

Power Management Policies
=========================

The power management subsystem supports the following power management policies:

* Residency based
* Application defined

The policy manager is responsible for informing the power subsystem which
power state the system should transition to based on states defined by the
platform and other constraints such as a list of allowed states.

More details on the states definition can be found in the
:dtcompatible:`zephyr,power-state` binding documentation.

Residency
---------

The power management system enters the power state which offers the highest
power savings, and with a minimum residency value (see
:dtcompatible:`zephyr,power-state`) less than or equal to the scheduled system
idle time duration.

This policy also accounts for the time necessary to become active
again. The core logic used by this policy to select the best power
state is:

.. code-block:: c

   if (time_to_next_scheduled_event >= (state.min_residency_us + state.exit_latency))) {
      return state
   }

Application
-----------

The application defines the power management policy by implementing the
:c:func:`pm_policy_next_state` function. In this policy the application is free
to decide which power state the system should transition to based on the
remaining time for the next scheduled timeout.

An example of an application that defines its own policy can be found in
:zephyr_file:`tests/subsys/pm/power_mgmt/`.

Policy and Power States
------------------------

The power management subsystem allows different Zephyr components and
applications to configure the policy manager to block system from transitioning
into certain power states. This can be used by devices when executing tasks in
background to prevent the system from going to a specific state where it would
lose context.
