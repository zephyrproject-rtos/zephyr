Device Power Management
#######################

Introduction
************

Device power management (PM) on Zephyr is a feature that enables devices to
save energy when they are not being used. This feature can be enabled by
setting :kconfig:option:`CONFIG_PM_DEVICE` to ``y``. When this option is
selected, device drivers implementing power management will be able to take
advantage of the device power management subsystem.

Zephyr supports two methods of device power management:

 - :ref:`Device Runtime Power Management <pm-device-runtime-pm>`
 - :ref:`System-Managed Device Power Management <pm-device-system-pm>`

.. _pm-device-runtime-pm:

Device Runtime Power Management
===============================

Device runtime power management involves coordinated interaction between
device drivers, subsystems, and applications. While device drivers
play a crucial role in directly controlling the power state of
devices, the decision to suspend or resume a device can also be
influenced by higher layers of the software stack.

Each layer—device drivers, subsystems, and applications—can operate
independently without needing to know about the specifics of the other
layers because the subsystem uses reference count to check when it needs
to suspend or resume a device.

- **Device drivers** are responsible for managing the
  power state of devices. They interact directly with the hardware to
  put devices into low-power states (suspend) when they are not in
  use, and bring them back (resume) when needed. Drivers should use the
  :ref:`device runtime power management APIs <device_runtime_apis>` provided
  by Zephyr to control the power state of devices.

- **Subsystems**, such as sensors, file systems,
  and network, can also influence device power management.
  Subsystems may have better knowledge about the overall system
  state and workload, allowing them to make informed decisions about
  when to suspend or resume devices. For example, a networking
  subsystem may decide to keep a network interface powered on if it
  expects network activity in the near future.

- **Applications** running on Zephyr can impact device
  power management as well. An application may have specific
  requirements regarding device usage and power consumption. For
  example, an application that streams data over a network may need
  to keep the network interface powered on continuously.

Coordination between device drivers, subsystems, and applications is
key to efficient device power management. For example, a device driver
may not know that a subsystem will perform a series of sequential
operations that require a device to remain powered on. In such cases,
the subsystem can use device runtime power management to ensure that
the device remains in an active state until the operations are
complete.

When using this Device Runtime Power Management, the System Power
Management subsystem is able to change power states quickly because it
does not need to spend time suspending and resuming devices that are
runtime enabled.

For more information, see :ref:`pm-device-runtime`.

.. _pm-device-system-pm:

System-Managed Device Power Management
======================================

The system managed device power management (PM) framework is a method where
devices are suspended along with the system entering a CPU (or SoC) power state.
It can be enabled by setting :kconfig:option:`CONFIG_PM_DEVICE_SYSTEM_MANAGED`.
When using this method, device power management is mostly done inside
:c:func:`pm_system_suspend()`.

If a decision to enter a CPU lower power state is made, the power management
subsystem will check if the selected low power state triggers device power
management and then suspend devices before changing state. The subsystem takes
care of suspending devices following their initialization order, ensuring that
possible dependencies between them are satisfied. As soon as the CPU wakes up
from a sleep state, devices are resumed in the opposite order that they were
suspended.

The decision about suspending devices when entering a low power state is done based on the
state and if it has set the property ``zephyr,pm-device-disabled``. Here is
an example of a target with two low power states with only one triggering device power
management:

.. code-block:: devicetree

   /* Node in a DTS file */
   cpus {
        power-states {
                state0: state0 {
                        compatible = "zephyr,power-state";
                        power-state-name = "standby";
                        min-residency-us = <5000>;
                        exit-latency-us = <240>;
                        zephyr,pm-device-disabled;
                };
                state1: state1 {
                        compatible = "zephyr,power-state";
                        power-state-name = "suspend-to-ram";
                        min-residency-us = <8000>;
                        exit-latency-us = <360>;
                };
        };
   };

.. note::

   When using :ref:`pm-system`, device transitions can be run from the idle thread.
   As functions in this context cannot block, transitions that intend to use blocking
   APIs **must** check whether they can do so with :c:func:`k_can_yield`.

This method of device power management can be useful in the following scenarios:

- Systems with no device requiring any blocking operations when suspending and
  resuming. This implementation is reasonably simpler than device runtime
  power management.
- For devices that can not make any power management decision and have to be
  always active. For example a firmware using Zephyr that is controlled by an
  external entity (e.g Host CPU). In this scenario, some devices have to be
  always active and should be suspended together with the SoC when requested by
  this external entity.

It is important to emphasize that this method has drawbacks (see above) and
:ref:`Device Runtime Power Management <pm-device-runtime-pm>` is the
**preferred** method for implementing device power management.

.. note::

    When using this method of device power management, the CPU will not
    enter a low-power state if a device cannot be suspended. For example,
    if a device returns an error such as ``-EBUSY`` in response to the
    ``PM_DEVICE_ACTION_SUSPEND`` action, indicating it is in the middle of
    a transaction that cannot be interrupted. Another condition that
    prevents the CPU from entering a low-power state is if the option
    :kconfig:option:`CONFIG_PM_NEED_ALL_DEVICES_IDLE` is set and a device
    is marked as busy.

.. note::

   Devices are suspended only when the last active core is entering a low power
   state and devices are resumed by the first core that becomes active.

Device Power Management States
******************************

The power management subsystem defines device states in
:c:enum:`pm_device_state`. This method is used to track power states of
a particular device. It is important to emphasize that, although the
state is tracked by the subsystem, it is the responsibility of each device driver
to handle device actions(:c:enum:`pm_device_action`) which change device state.

Each :c:enum:`pm_device_action` have a direct an unambiguous relationship with
a :c:enum:`pm_device_state`.

.. graphviz::
   :caption: Device actions x states

    digraph {
        node [shape=circle];
        rankdir=LR;
        subgraph {

            SUSPENDED [label=PM_DEVICE_STATE_SUSPENDED];
            SUSPENDING [label=PM_DEVICE_STATE_SUSPENDING];
            ACTIVE [label=PM_DEVICE_STATE_ACTIVE];
            OFF [label=PM_DEVICE_STATE_OFF];


            ACTIVE -> SUSPENDING -> SUSPENDED;
            ACTIVE -> SUSPENDED ["label"="PM_DEVICE_ACTION_SUSPEND"];
            SUSPENDED -> ACTIVE ["label"="PM_DEVICE_ACTION_RESUME"];

            {rank = same; SUSPENDED; SUSPENDING;}

            OFF -> SUSPENDED ["label"="PM_DEVICE_ACTION_TURN_ON"];
            SUSPENDED -> OFF ["label"="PM_DEVICE_ACTION_TURN_OFF"];
            ACTIVE -> OFF ["label"="PM_DEVICE_ACTION_TURN_OFF"];
        }
    }

As mentioned above, device drivers do not directly change between these states.
This is entirely done by the power management subsystem. Instead, drivers are
responsible for implementing any hardware-specific tasks needed to handle state
changes.

Device Model with Power Management Support
******************************************

Drivers initialize devices using macros. See :ref:`device_model_api` for
details on how these macros are used. A driver which implements device power
management support must provide these macros with arguments that describe its
power management implementation.

Use :c:macro:`PM_DEVICE_DEFINE` or :c:macro:`PM_DEVICE_DT_DEFINE` to define the
power management resources required by a driver. These macros allocate the
driver-specific state which is required by the power management subsystem.

Drivers can use :c:macro:`PM_DEVICE_GET` or
:c:macro:`PM_DEVICE_DT_GET` to get a pointer to this state. These
pointers should be passed to ``DEVICE_DEFINE`` or ``DEVICE_DT_DEFINE``
to initialize the power management field in each :c:struct:`device`.

Here is some example code showing how to implement device power management
support in a device driver.

.. code-block:: c

    #define DT_DRV_COMPAT dummy_device

    static int dummy_driver_pm_action(const struct device *dev,
                                      enum pm_device_action action)
    {
        switch (action) {
        case PM_DEVICE_ACTION_SUSPEND:
            /* suspend the device */
            ...
            break;
        case PM_DEVICE_ACTION_RESUME:
            /* resume the device */
            ...
            break;
        case PM_DEVICE_ACTION_TURN_ON:
            /*
             * powered on the device, used when the power
             * domain this device belongs is resumed.
             */
            ...
            break;
        case PM_DEVICE_ACTION_TURN_OFF:
            /*
             * power off the device, used when the power
             * domain this device belongs is suspended.
             */
            ...
            break;
        default:
            return -ENOTSUP;
        }

        return 0;
    }

    PM_DEVICE_DT_INST_DEFINE(0, dummy_driver_pm_action);

    DEVICE_DT_INST_DEFINE(0, &dummy_init,
        PM_DEVICE_DT_INST_GET(0), NULL, NULL, POST_KERNEL,
        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

.. _pm-device-shell:

Shell Commands
**************

Power management actions can be triggered from shell commands for testing
purposes. To do that, enable the :kconfig:option:`CONFIG_PM_DEVICE_SHELL`
option and issue a ``pm`` command on a device from the shell, for example:

.. code-block:: console

        uart:~$ device list
        - buttons (active)
        uart:~$ pm suspend buttons
        uart:~$ device list
        devices:
        - buttons (suspended)

To print the power management state of a device, enable
:kconfig:option:`CONFIG_DEVICE_SHELL` and use the ``device list`` command, for
example:

.. code-block:: console

        uart:~$ device list
        devices:
        - i2c@40003000 (active)
        - buttons (active, usage=1)
        - leds (READY)

In this case, ``leds`` does not support PM, ``i2c`` supports PM with manual
suspend and resume actions and it's currently active, ``buttons`` supports
runtime PM and it's currently active with one user.

.. _pm-device-busy:

Busy Status Indication
**********************

When the system is idle and the SoC is going to sleep, the power management
subsystem can suspend devices, as described in :ref:`pm-device-system-pm`. This
can cause device hardware to lose some states. Suspending a device which is in
the middle of a hardware transaction, such as writing to a flash memory, may
lead to undefined behavior or inconsistent states. This API guards such
transactions by indicating to the kernel that the device is in the middle of an
operation and should not be suspended.

When :c:func:`pm_device_busy_set` is called, the device is marked as busy and
the system will not do power management on it. After the device is no
longer doing an operation and can be suspended, it should call
:c:func:`pm_device_busy_clear`.

.. _pm-device-constraint:

Device Power Management X System Power Management
*************************************************

When managing power in embedded systems, it's crucial to understand
the interplay between device power state and the overall system power
state. Some devices may have dependencies on the system power
state. For example, certain low-power states of the SoC might not
supply power to peripheral devices, leading to problems if the device
is in the middle of an operation. Proper coordination is essential to
maintain system stability and data integrity.

To avoid this sort of problem, devices must :ref:`get and release lock <pm-policy-power-states>`
power states that cause power loss during an operation.

Zephyr provides a mechanism for devices to declare which power states cause power
loss and an API that automatically get and put lock on them. This feature is
enabled setting :kconfig:option:`CONFIG_PM_POLICY_DEVICE_CONSTRAINTS` to ``y``.

Once this feature is enabled, devices must declare in devicetree which
states cause power loss. In the following example, device ``test_dev``
says that power states ``state1`` and ``state2`` cause power loss.

.. code-block:: devicetree

    power-states {
            state0: state0 {
                    compatible = "zephyr,power-state";
                    power-state-name = "suspend-to-idle";
                    min-residency-us = <10000>;
                    exit-latency-us = <100>;
            };

            state1: state1 {
                    compatible = "zephyr,power-state";
                    power-state-name = "standby";
                    min-residency-us = <20000>;
                    exit-latency-us = <200>;
            };

            state2: state2 {
                    compatible = "zephyr,power-state";
                    power-state-name = "suspend-to-ram";
                    min-residency-us = <50000>;
                    exit-latency-us = <500>;
            };

            state3: state3 {
                    compatible = "zephyr,power-state";
                    power-state-name = "suspend-to-ram";
                    status = "disabled";
            };
    };

    test_dev: test_dev {
            compatible = "test-device-pm";
            status = "okay";
            zephyr,disabling-power-states = <&state1 &state2>;
    };

After that devices can lock these state calling
:c:func:`pm_policy_device_power_lock_get` and release with
:c:func:`pm_policy_device_power_lock_put`. For example:

.. code-block:: C

    static void timer_expire_cb(struct k_timer *timer)
    {
           struct test_driver_data *data = k_timer_user_data_get(timer);

           data->ongoing = false;
           k_timer_stop(timer);
           pm_policy_device_power_lock_put(data->self);
    }

    void test_driver_async_operation(const struct device *dev)
    {
           struct test_driver_data *data = dev->data;

           data->ongoing = true;
           pm_policy_device_power_lock_get(dev);

           /** Lets set a timer big enough to ensure that any deep
            *  sleep state would be suitable but constraints will
            *  make only state0 (suspend-to-idle) will be used.
            */
           k_timer_start(&data->timer, K_MSEC(500), K_NO_WAIT);
    }

Wakeup capability
*****************

Some devices are capable of waking the system up from a sleep state.
When a device has such capability, applications can enable or disable
this feature on a device dynamically using
:c:func:`pm_device_wakeup_enable`.

This property can be set on device declaring the property ``wakeup-source`` in
the device node in devicetree. For example, this devicetree fragment sets the
``gpio0`` device as a "wakeup" source:

.. code-block:: devicetree

                gpio0: gpio@40022000 {
                        compatible = "ti,cc13xx-cc26xx-gpio";
                        reg = <0x40022000 0x400>;
                        interrupts = <0 0>;
                        status = "disabled";
                        label = "GPIO_0";
                        gpio-controller;
                        wakeup-source;
                        #gpio-cells = <2>;
                };

By default, "wakeup" capable devices do not have this functionality enabled
during the device initialization. Applications can enable this functionality
later calling :c:func:`pm_device_wakeup_enable`.

.. note::

   This property is **only** used by the system power management to identify
   devices that should not be suspended.
   It is responsibility of driver or the application to do any additional
   configuration required by the device to support it.

Examples
********

Some helpful examples showing device power management features:

* :zephyr_file:`samples/subsys/pm/device_pm/`
* :zephyr_file:`tests/subsys/pm/power_mgmt/`
* :zephyr_file:`tests/subsys/pm/device_wakeup_api/`
* :zephyr_file:`tests/subsys/pm/device_driver_init/`
