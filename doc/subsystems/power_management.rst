.. _power_management:

Power Management
################

The power management infrastructure consists of interfaces exported by the
power management subsystem.  This subsystem exports interfaces that a
:abbr:`Power Management Application (PMA)` uses to implement power management
policies.

Terminology
***********

:dfn:`PMA`

   The system integrator provides the :abbr:`PMA (Power Manager
   Application)`. The PMA maintains any power management policies and
   executes the power management actions based on those policies.
   The PMA must be integrated into the main Zephyr application.

:dfn:`LPS`

   :abbr:`LPS (Low Power States)` refers to any one of the low power states supported by the CPU.

:dfn:`SoC Power State`

   An SoC Power State describes processor and device power statuses
   implemented at the SoC level.

:dfn:`Hook function`

   A Hook function is a callback function that one component implements and
   another component calls. For example, the PMA implements functions that
   the kernel calls.

Architecture and SoC dependent Power States:
============================================

On x86:
-------

   `Active`
      The CPU is active and running in the hardware defined C0 C-state.

   `Idle`
      The CPU is not active but continues to be powered.
      The CPU may be in one of any lower C-states: C1, C2, etc.

   `Deep Sleep`
      The Power is off to the processor and system clock. RAM is retained.

On ARM
------

    `Active`
        The CPU is active and running.

    `Idle`
        Stops the processor clock. The ARM documentation describes
        this as *Sleep*.

    `Deep Sleep`
        Stops the system clock and switches off the PLL and flash
        memory. RAM is retained.

On ARC
------

    `Active`
        The CPU is currently active and running in the SS0 state.

    `Idle`
        Defined as the SS1 and SS2 states.

The power states described here are generic terms that map to the power
states commonly supported by processors and SoCs based on the three
architectures. When coding a PMA, please refer to the data sheet of the SoC
to get details on each power state.

Overview
********

The Zephyr power management subsystem provides interfaces that a system
integrator can use to create a PMA. The PMA then enforces any policies
needed. The design is based on the philosophy of not enforcing any policies
in the kernel giving full flexibility to the PMA.

The provided infrastructure has an architecture independent interface. Both
the microkernel and nanokernel notify the PMA when the kernel is about to
enter or exit a system idle state. The PMA can perform the power management
policy operations during these notifications.

Policies
********

When the power management subsystem notifies the PMA that the kernel is about
to enter a system idle state, it specifies the period of time the system
intends to stay idle. The PMA performs any power management operations during
this time. The PMA can perform various operations. For example, put the
processor or the SoC in a low power state, turn off some or all of the
peripherals, and gate device clocks. Using combinations of these operations,
the PMA can create fine grain custom power management policies.

Different levels of power savings and different wake latencies characterize
these fine grain policies. In general, operations that save more power have a
higher wake latency. When making policy decisions, the PMA chooses the
policy that saves the most power. At the same time, the policy's total
execution time must fit well within the idle time allotted by the power
management subsystem.

The Zephyr power management subsystem classifies policies into categories
based on relative power savings and the corresponding wake latencies. These
policies also loosely map to common processor and SoC power states in the
supported architectures. The PMA should map the fine grain custom policies to
the policy categories of the power management subsystem. The power management
subsystem defines three categories:

* SYS_PM_LOW_POWER_STATE
* SYS_PM_DEEP_SLEEP
* SYS_PM_DEVICE_SUSPEND_ONLY

SYS_PM_LOW_POWER_STATE
======================

In this policy category, the PMA performs power management operations on some
or all devices and puts the processor into a low power state. The device
power management operations can involve turning off peripherals and gating
device clocks. When any of those operations causes the device registers to
lose their state, then those states must be saved and restored. The PMA
should map fine grain policies with relatively less wake latency to this
category. Policies with larger wake latency should be mapped to the
`SYS_PM_DEEP_SLEEP`_ category. Policies in this category exit from an
external interrupt, a wake up event set by the PMA, or when the idle time
alloted by the power management subsystem expires.

SYS_PM_DEEP_SLEEP
=================

In this policy category, the PMA puts the system into the deep sleep power
states supported by SoCs. In this state, the system clock is turned off. The
processor is turned off and loses its state. RAM is expected to be retained
and can save and restore processor states. Only the devices necessary to wake
up the system from the deep sleep power state stay on. The SoC turns off the
power to all other devices. Since this causes device registers to lose their
state, they must be saved and restored. The PMA should map fine grain
policies with the highest wake latency to this policy category. Policies in
this category exit from SoC dependent wake events.

SYS_PM_DEVICE_SUSPEND_ONLY
==========================

In this policy category, the PMA performs power management operations on some
devices but none that result in a processor or SoC power state transition.
The PMA should map its fine grain policies that have the lowest wake latency
to this policy category. Policies in this category exit from an external
interrupt or when the idle time alloted by the power management subsystem
expires.

Some policy categories names are similar to the power states of processors or
SoCs, for example, :code:`SYS_PM_DEEP_SLEEP`. However, they must be seen
as policy categories and do not indicate any specific processor or SoC power
state by themselves.

.. _pm_hook_infra:

Power Management Hook Infrastructure
************************************

This infrastructure consists of the hook functions that the PMA implemented.
The power management subsystem calls these hook functions when the kernel
enters and exits the idle state, in other words, when the kernel has nothing
to schedule. This section provides a general overview and general concepts of
the hook functions. Refer to :ref:`power_management_api` for the detailed
description of the APIs.

Suspend Hook function
=====================

.. code-block:: c

   int _sys_soc_suspend(int32_t ticks);

When the kernel is about to go idle, the power management subsystem calls the
:code:`_sys_soc_suspend()` function, notifying the PMA that the kernel is
ready to enter the idle state.

At this point, the kernel has disabled interrupts and computed the maximum
number of ticks the system can remain idle. The function passes the time that
the system can remain idle to the PMA along with the notification. When
notified, the PMA selects and executes one of the fine grain power policies
that can be executed within the allotted time.

The power management subsystem expects the :code:`_sys_soc_suspend()` to
return one of the following values based on the power management operations
the PMA executed:

:code:`SYS_PM_NOT_HANDLED`

   No power management operations. Indicates that the PMA could not
   accomplish any actions in the time allotted by the kernel.

:code:`SYS_PM_DEVICE_SUSPEND_ONLY`

   Only devices are suspended. Indicates that the PMA could accomplish any
   device suspend operations. These operations do not include any processor
   or SOC power operations.

:code:`SYS_PM_LOW_POWER_STATE`

   Entered a LPS. Indicates that the PMA could put the processor into a low
   power state.

:code:`SYS_PM_DEEP_SLEEP`

   Entered deep sleep. Indicates that the PMA could put the SoC in a deep
   sleep state.

Resume Hook function
====================

.. code-block:: c

   void _sys_soc_resume(void);

The kernel calls this hook function when exiting from an idle state or a low
power state. Based on which policy the PMA executed in the
:code:`_sys_soc_suspend()` function, the PMA performs the necessary recovery
operations in this hook function.

Since the hook functions are called with the interrupts disabled, the PMA
should ensure that its operations are completed quickly. Thus, the PMA
ensures that the kernel's scheduling performance is not disrupted.

Device Power Management Infrastructure
**************************************

The device power management infrastructure consists of interfaces to the
Zephyr device model. These interfaces enable the PMA to suspend and resume
operations on devices. Refer to :ref:`power_management_api` for detailed
description of the APIs.

Device Power Management Operations
==================================

Drivers can implement handlers to suspend and resume power management
operations. The PMA performs the necessary power management operations on the
devices by calling each of the suspend and resume handler functions of the
drivers.

Operations Structure
--------------------

.. code-block:: c

    struct device_pm_ops {
            int (*suspend)(struct device *device, int pm_policy);
            int (*resume)(struct device *device, int pm_policy);
    };

This structure contains pointers to the :c:func:`suspend()` and
:c:func:`resume()` handler functions. The device driver initializes those
pointers with the corresponding handler functions implemented in the
driver.

Default Initializer Function
----------------------------

.. code-block:: c

   int device_pm_nop(struct device *unused_device, int unused_policy);

When the driver does not implement any operations, the driver can initialize
the corresponding pointer with this function. This default initializer
function does nothing and should be used instead of implementing a dummy
function in the driver to avoid wasting code memory.

.. _dev_suspend_handler:

Device Suspend Operation Handler Function
-----------------------------------------

.. code-block:: c

   int suspend(struct device *device, int pm_policy);

The device driver implements this function to perform the suspend operations
on the handled devices. The PMA calls the function and passes the power
policy to execute. The device driver performs the operations necessary to
handle the transitions associated with the policy the PMA specified. Here are
some example operations that the device driver performs:

* Save device states.

* Gate clocks.

* Turn off peripherals.

This function returns 0 if successful. Otherwise, the function returns an
appropriate negative `errno` value.

.. _dev_resume_handler:

Device Resume Operation Handler Function
----------------------------------------

.. code-block:: c

   int resume(struct device *device, int pm_policy);

The PMA calls this function to resume operations on the devices on which it
had previously performed suspend operations. The device driver performs the
necessary resume operations on its device, following the power management
policy specified in the function's argument.

The function returns 0 if successful. In all other cases it returns an
appropriate negative :code:`errno` value.

Device Model with Power Management Support
==========================================

Drivers initialize the devices using macros. See :ref:`device_drivers` for
details on how these macros are used. Extended versions of the macros are
provided for drivers with power management support. The macros take
additional parameters that are necessary to initialize pointers to the power
management related handler functions.

These macros should be used:

:code:`DEVICE_AND_API_INIT_PM`

   Use this macro in place of the :code:`DEVICE_AND_API_INIT` macro.

:code:`DEVICE_INIT_PM`

   Use this macro in place of the :code:`DEVICE_INIT` macro.

:code:`SYS_INIT_PM`

   Use this macro in place of the :code:`SYS_INIT` macro.

Device Power Management API for the PMA
=======================================

The PMA uses these APIs to perform suspend and resume operations on the
devices.

Get Device List
---------------

.. code-block:: c

   void device_list_get(struct device **device_list, int *device_count);

The Zephyr kernel internally maintains a list of all devices in the system.
The PMA uses this API to get the device list. The PMA can use the list to
identify the devices on which to execute power management operations.

The PMA can use this list to create a sorted order list based on device
dependencies. The PMA creates device groups to execute different policies
on each device group.

.. note::

   Ensure that the PMA does not alter the original list. Since the kernel
   uses the original list, it should remain unchanged.

PMA Device Suspend API
----------------------

.. code-block:: c

   int device_suspend(struct device *device, int pm_policy);

Calls the :c:func:`suspend()` handler function implemented by the device
driver. See :ref:`dev_suspend_handler` for more information.

PMA Device Resume API
---------------------

.. code-block:: c

   int device_resume(struct device *device, int pm_policy);

Calls the :c:func:`resume()` handler function implemented by the device
driver. See :ref:`dev_resume_handler` for more information.

Busy Status Indication
======================

The PMA executes some power policies that can turn off power to devices,
causing them to lose their state. If the devices are in the middle of some
hardware transaction, like writing to flash memory when the power is turned
off, then such transactions would be left in an inconsistent state. This
infrastructure guards such transactions by indicating to the PMA that
the device is in the middle of a hardware transaction.

When the :code:`_sys_soc_suspend()` is called, the PMA checks if any device
is busy. The PMA can then decide to execute a policy other than deep sleep or
to defer power management operations until the next call of
:code:`_sys_soc_suspend()`.

If other recovery or retrieval methods are in place, the driver can avoid
guarding the transactions. Not all hardware transactions must be guarded. The
Zephyr kernel provides the following APIs for the device drivers and the PMA
to decide whether a particular transaction must be guarded.

Indicate Busy Status API
------------------------

.. code-block:: c

   void device_busy_set(struct device *busy_dev);

Sets a bit corresponding to the device, in a data structure maintained by the
kernel, to indicate whether or not it is in the middle of a transaction.

Clear Busy Status API
---------------------

.. code-block:: c

   void device_busy_clear(struct device *busy_dev);

Clears the bit corresponding to the device in a data structure
maintained by the kernel to indicate that the device is not in the middle of
a transaction.

Check Busy Status of Single Device API
--------------------------------------

.. code-block:: c

   int device_busy_check(struct device *chk_dev);

Checks whether a device is busy. The API returns 0 if the device
is not busy.

Check Busy Status of All Devices API
------------------------------------

.. code-block:: c

   int device_any_busy_check(void);

Checks if any device is busy. The API returns 0 if no device in the system is busy.

.. _pm_config_flags:

Power Management Configuration Flags
************************************

The Power Management features can be individually enabled and disabled using
the following configuration flags.

:code:`CONFIG_SYS_POWER_MANAGEMENT`

   This flag enables the power management subsystem.

:code:`CONFIG_SYS_POWER_LOW_POWER_STATE`

   The PMA enables this flag to use the :code:`SYS_PM_LOW_POWER_STATE` policy.

:code:`CONFIG_SYS_POWER_DEEP_SLEEP`

   This flag enables support for the :code:`SYS_PM_DEEP_SLEEP` policy.

:code:`CONFIG_DEVICE_POWER_MANAGEMENT`

   This flag is enabled if the PMA and the devices support device power
   management.

Writing a Power Management Application
**************************************

A typical PMA executes policies through power management APIS.  This section
details various scenarios that can be used to help developers write their own
custom PMAs.

The PMA is part of a larger application doing more than just power
management. This section focuses on the power management aspects of the
application.

Initial Setup
=============

To enable the power management support, the application must do the following:

#. Enable the :code:`CONFIG_SYS_POWER_MANAGEMENT` flag

#. Enable other required config flags described in :ref:`pm_config_flags`.

#. Implement the hook functions described in :ref:`pm_hook_infra`.

Device List and Policies
========================

The PMA retrieves the list of enabled devices in the system using the
:c:func:`device_list_get()` function. Since the PMA is part of the
application, the PMA starts after all devices in the system have been
initialized. Thus, the list of devices will not change once the application
has begun.

Once the device list has been retrieved and stored, the PMA can form device
groups and sorted lists based on device dependencies. The PMA uses the device
lists and the known aggregate wake latency of the combination of power
operations to create the fine grain custom power policies. Finally, the PMA
maps these custom policies to the policy categories defined by the power
management subsystem as described in `Policies`_.

Scenarios During Suspend
========================

When the power management subsystem calls the :code:`_sys_soc_suspend()`
function, the PMA can select between multiple scenarios.

Scenario 1
----------

The time allotted is too short for any power management.

In this case, the PMA leaves the interrupts disabled, and returns the code
:code:`SYS_PM_NOT_HANDLED`. This actions allow the Zephyr kernel to continue
with its normal idling process.

Scenario 2
----------

The time allotted allows the suspension of some devices.

The PMA scans through the devices that meet the criteria and calls the
:c:func:`device_suspend()` function for each device.

After all devices are suspended properly, the PMA executes the following
operations:

* If the time allotted is enough for the :code:`SYS_PM_LOW_POWER_STATE`
  policy:

   #. The PMA sets up the wake event, puts the CPU in a LPS, and re- enables
      the interrupts at the same time.

   #. The PMA returns the :code:`SYS_PM_LOW_POWER_STATE` code.

* If the time allotted is not enough for the :code:`SYS_PM_LOW_POWER_STATE`
  policy, the PMA returns the :code:`SYS_PM_DEVICE_SUSPEND_ONLY` code.

When a device fails to suspend, the PMA executes the following operations:

* If the system integrator determined that the device is not essential to the
  suspend process, the PMA can ignore the failure.

* If the system integrator determined that the device is essential to the
  suspend process, the PMA takes any necessary recovery actions and
  returns the :code:`SYS_PM_NOT_HANDLED` code.

Scenario 3
----------

The time allotted is enough for all devices to be suspended.

The PMA calls the :c:func:`device_suspend()` function for each device.

After all devices are suspended properly and the time allotted is enough for
the :code:`SYS_PM_DEEP_SLEEP` policy, the PMA executes the following
operations:

#. Calls the :c:func:`device_any_busy_check()` function to get device busy
   status. If any device is busy, the PMA must choose a policy other than
   :code:`SYS_PM_DEEP_SLEEP`.
#. Sets up wake event.
#. Puts the SOC in the deep sleep state.
#. Re-enables interrupts.
#. Returns the :code:`SYS_PM_DEEP_SLEEP` code.

If, on the other hand, the time allotted is only enough for the
:code:`SYS_PM_LOW_POWER_STATE` policy, The PMA executes the following
operations:

#. Sets up wake event.
#. Puts the CPU in a LPS re-enabling interrupts at the same time.
#. Returns the :code:`SYS_PM_LOW_POWER_STATE` code.

If the time allotted is not enough for any CPU or SOC power management
operations, the PMA returns the :code:`SYS_PM_DEVICE_SUSPEND_ONLY` code.

When a device fails to suspend, the PMA executes the following operations:

* If the system integrator determined that the device is not essential to the
  suspend process the PMA can ignore the failure.

* If the system integrator determined that the device is essential to the
  suspend process, the PMA takes any necessary recovery actions and
  returns the :code:`SYS_PM_NOT_HANDLED` code.

Policy Decision Summary
=======================

+---------------------------------+---------------------------------------+
| PM operations                   | Policy and Return Code                |
+=================================+=======================================+
| Suspend some devices and        | :code:`SYS_PM_LOW_POWER_STATE`        |
|                                 |                                       |
| Enter Low Power State           |                                       |
+---------------------------------+---------------------------------------+
| Suspend all devices and         | :code:`SYS_PM_LOW_POWER_STATE`        |
|                                 |                                       |
| Enter Low Power State           |                                       |
+---------------------------------+---------------------------------------+
| Suspend all devices and         | :code:`SYS_PM_DEEP_SLEEP`             |
|                                 |                                       |
| Enter Deep Sleep                |                                       |
+---------------------------------+---------------------------------------+
| Suspend some or all devices and | :code:`SYS_PM_DEVICE_SUSPEND_ONLY`    |
|                                 |                                       |
| No CPU/SoC PM Operation         |                                       |
+---------------------------------+---------------------------------------+
| No PM operation                 | :code:`SYS_PM_NOT_HANDLED`            |
+---------------------------------+---------------------------------------+
