Device Power Management
#######################

Introduction
************

Device power management (PM) on Zephyr is a feature that enables devices to
save energy when they are not being used. This feature can be enabled by
setting :kconfig:option:`CONFIG_PM_DEVICE` to ``y``. When this option is
selected, device drivers implementing power management will be able to take
advantage of the device power management subsystem.

Zephyr supports two types of device power management:

 - :ref:`Device Runtime Power Management <pm-device-runtime-pm>`
 - :ref:`System-Managed Device Power Management <pm-device-system-pm>`

.. _pm-device-runtime-pm:

Device Runtime Power Management
*******************************

In this method, the application or any component that deals with devices directly
and has the best knowledge of their use, performs the device power management. This
saves power if some devices that are not in use can be turned off or put
in power saving mode. This method allows saving power even when the CPU is
active. The components that use the devices need to be power aware and should
be able to make decisions related to managing device power.

When using this type of device power management, the kernel can change CPU
power states quickly when :c:func:`pm_system_suspend()` gets called. This is
because it does not need to spend time doing device power management if the
devices are already put in the appropriate power state by the application or
component managing the devices.

For more information, see :ref:`pm-device-runtime`.

.. _pm-device-system-pm:

System-Managed Device Power Management
**************************************

When using this type, device power management is mostly done inside
:c:func:`pm_system_suspend()` along with entering a CPU or SOC power state.

If a decision to enter a CPU lower power state is made, the power management
subsystem will suspend devices before changing state. The subsystem takes care
of suspending devices following their initialization order, ensuring that
possible dependencies between them are satisfied. As soon as the CPU wakes up
from a sleep state, devices are resumed in the opposite order that they were
suspended.

.. note::

   When using :ref:`pm-system`, device transitions can be run from the idle thread.
   As functions in this context cannot block, transitions that intend to use blocking
   APIs **must** check whether they can do so with :c:func:`k_can_yield`.

This type of device power management can be useful when the application is not
power aware and does not implement runtime device power management. Though,
:ref:`Device Runtime Power Management <pm-device-runtime-pm>` is the **preferred**
option for device power management.

.. note::

    When using this type of device power management, the CPU will only enter
    a low power state only if no device is in the middle of a hardware
    transaction that cannot be interrupted.

.. note::

    This type of device power management is disabled when
    :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_EXCLUSIVE` is set to ``y`` (that is
    the default value when :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` is enabled)

.. note::

   Devices are suspended only when the last active core is entering a low power
   state and devices are resumed by the first core that becomes active.

Device Power Management States
******************************

The power management subsystem defines device states in
:c:enum:`pm_device_state`. This type is used to track power states of
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

Power Domain
************

Power domain on Zephyr is represented as a regular device. The power management
subsystem ensures that a domain is resumed before and suspended after devices
using it. For more details, see :ref:`pm-power-domain`.
