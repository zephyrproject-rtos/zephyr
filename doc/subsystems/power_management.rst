.. _power_management

Power Management
################

The power management infrastructure consists of interfaces exported by the
power management subsystem for a Power Manager application to implement power
policies.

Terminology
***********

`PMA`
    Shortened form for Power Manager Application, the system integrator
    provided application that maintains any power management policies and
    enforces power management actions based on them. This would be integrated
    into the main Zephyr application.

`LPS`
    Any of the CPU low power states supported by the processor.

`SoC Power State`
    Power state implemented at the SoC level which includes processor and
    device power state.

`Hook function`
    A callback function implemented by one component and called by another
    component. e.g. functions implemented by PMA that can be called by the
    kernel.

**Architecture and SoC dependent Power States:**

**On x86:**
    `Active`
        The CPU is currently active and running in the hardware defined
        C0
    `Idle`
        The CPU is not currently active, and continues to be powered
        on. The CPU may be in one of any lower C-states (i.e. C1, C2, etc).
    `Deep Sleep`
        Power to the processor and system clock turned off. RAM
        retained.
**On ARM:**
    `Active`
        The CPU is currently active and running
    `Idle`
        Stops the processor clock.  The ARM documentation describes
        this as Sleep.
    `Deep Sleep`
        Stops the system clock and switches off the PLL and flash
        memory. RAM retained.
**On ARC:**
    `Active`
        The CPU is currently active and running in the SS0 state.
    `Idle`
        Defined as the SS1, SS2 states.

.. note::
    These power states are generic terms that map to the power states commonly
    supported by processors and SoCs based on the 3 architectures. The PMA
    writer should refer to the data sheet of the SoC to get details of each
    power state.


Overview
********
The Zephyr power management subsystem provides interfaces for a system
integrator to utilize and create their own Power Management Application (PMA)
that can enforce any policies needed.  It is based on the philosophy of not
enforcing any policies in the kernel and thus giving full flexibility to the
PMA.

This will be accomplished by providing an infrastructure that has an
architecture independent interface.  The Zephyr kernel will provide
notification methods in both the Microkernel and Nanokernel for the PMA when
the OS is about to enter and exit system idle. The PMA will do various power
management policy enforcement operations during these notifications.

Policies
********
When the Power Management subsystem notifies the PMA that the kernel is about
to enter system idle, it would specify a time period that it intends to idle.
This time period is the alloted time for the PMA to do any power management
operations. The PMA has the choice to do various operations, like putting the
processor or SoC in low power states, turning off some or all peripherals and
gating device clocks.  Using various combinations of these operations, the PMA
would create fine grain custom power management policies. These fine grain
policies would be characterized by different levels of power savings and
different wake latencies.  Generally, the operations that save more power have
a higher wake latency. While making policy decisions, the PMA would choose the
policy that would save the most power and at the same time, its total
execution time fits well within the idle time allotted by the Power Management
subsystem.

The Zephyr Power Management subsystem classifies policies into categories based
on relative power savings and the corresponding wake latencies. These also
loosely map to common power states supported by processors and SoCs in the
supported architectures. The PMA should map its fine grain custom policies to
the policy categories defined by the Power Management subsystem listed below.

SYS_PM_LOW_POWER_STATE
======================
In this policy category, the PMA would do power management operations on some
or all devices and put the processor into a low power state. The device power
management operations may involve turning off peripherals and gating device
clocks. If any of those operations result in the device registers losing
state, then those states would need to be saved and restored. The PMA should
map fine grain policies to this category, with relatively less wake latency
than those that it maps to `SYS_PM_DEEP_SLEEP`_ category. The exit from this
policy is from an external interrupt, a wake up event set by the PMA or when
the idle time alloted by the Power Management subsystem expires.

SYS_PM_DEEP_SLEEP
=================
In this policy category, the PMA would put the system into the deep sleep
category of power states supported by SoCs. In this state, the system clock
would be turned off. The processor is turned off and would lose state. RAM is
expected to be retained and can be used to save and restore processor states.
Depending on the SoC, selected devices would also be turned off in the deep
sleep power state. Since this would cause device registers to lose state, they
would need to be saved and restored. The PMA should map fine grain policies
with the highest wake latency to this policy category. The exit from this
policy is from SoC dependent wake events.

SYS_PM_DEVICE_SUSPEND_ONLY
==========================
In this policy category, the PMA would do power management operations on some
devices but would not do any operations that would result in a processor or
SoC power state transition. The PMA should map its fine grain policies that have
the least wake latency to this category. Exit from this policy is from an
external interrupt or when the idle time alloted by the Power Management
subsystem expires.

.. note::

   Some of the policy categories are named similar to the processor or SoC power
   state that it is associated with. e.g. :c:macro:`SYS_PM_DEEP_SLEEP`. However,
   they should be seen as policy categories and are not intended to indicate any
   specific processor or SoC power state by itself.

Power Management Hook Infrastructure
************************************
This consists of hook functions that would be implemented by the PMA and would
be called by the power management subsystem when the kernel enters and exits
idle state i.e. when it has nothing to schedule. This section will give a
general overview and concepts of the hook functions. Refer to
:ref:`Power Management APIs <power_management_api>` for detailed description
of the APIs.

Suspend Hook function
=====================
.. code-block:: c

   int _sys_soc_suspend(int32_t ticks);

When kernel is about to go idle, the kernel itself will disable interrupts. The
kernel will then call the :c:func:`_sys_soc_suspend()` function, notifying the
PMA of the operation.  Included in the notification are the maximum number of
ticks that the system can be set idle for.  The PMA will then determine what
policies can be executed within the allotted time frame.

The kernel expects the :c:func:`_sys_soc_suspend()` to return one of the
following values based on the policy operation the PMA executed:

`SYS_PM_NOT_HANDLED`
    No PM operations. Indicating the PMA was not able to accomplish any
    action in the time allotted by the kernel.

`SYS_PM_DEVICE_SUSPEND_ONLY`
    Only Devices are suspended. Indicating that the PMA has accomplished
    any device suspend operations.  This does not include any CPU or SOC power
    operations.

`SYS_PM_LOW_POWER_STATE`
    Low Power State. Indicating that the PMA was successful at putting the
    CPU into a low power state.

`SYS_PM_DEEP_SLEEP`
    Deep Sleep. Indicating that the PMA was successful at pushing the SOC
    into the Deep Sleep state.

Resume Hook function
====================
.. code-block:: c

   void _sys_soc_resume(void);

This hook function will be called by the kernel when exiting from idle state or
from the low power state the PMA put the processor and SoC in. Based on the
policy executed by the PMA in :c:func:`_sys_soc_suspend()`, it should do the
necessary recovery operations in this function.

.. note::
    Since the hook functions are called with interrupts disabled, the PMA should
    ensure that it completes operations quick so that the kernel's scheduling
    performance is not disrupted.

Device Power Management Infrastructure
**************************************
This infrastructure consists of interfaces into the Zephyr device model which
enables the PMA to do suspend and resume operations on devices. Refer to
:ref:`Power Management APIs <power_management_api>` for detailed description of
the APIs.

Device Power Management Operations
==================================
Drivers would implement handlers for suspend and resume power management
operations. PMA would call each of the drivers suspend and resume handler
functions to do the necessary power management operations on those devices.

Device PM Operations structure
------------------------------
.. code-block:: c

    struct device_pm_ops {
            int (*suspend)(struct device *device, int pm_policy);
            int (*resume)(struct device *device, int pm_policy);
    };

This structure contains pointers to the :c:func:`suspend()` and
:c:func:`resume()` handler functions.  The device driver should initialize
those pointers with the corresponding handler functions that is implemented in
the driver.

Default Initializer function for PM ops
---------------------------------------
If the driver does not implement any of of the operations then it can
initialize the corresponding pointer with the following function that does
nothing. This function should be used instead of the driver implementing its
own dummy function to avoid wasting code memory.

.. code-block:: c

   int device_pm_nop(struct device *unused_device, int unused_policy);

Device Suspend Operation Handler function
-----------------------------------------
.. code-block:: c

   int suspend(struct device *device, int pm_policy);

This function is implemented by the device driver to perform suspend operations
on the devices it handles.  The PMA would call it passing the power policy it
is currently executing.  The device driver would do operations necessary to
handle the transitions associated with the policy specified by the PMA.
Example operations that the device driver would do are:

- Save device states

- Gate clocks

- Turn off peripherals

It would return 0 if successful.  In all other cases it would return an
appropriate negative :c:macro:`errno` value.


Device Resume Operation Handler function
----------------------------------------
.. code-block:: c

   int resume(struct device *device, int pm_policy);

PMA would call this when it is doing resume operations on devices.  The device
driver would do the necessary resume operation on its device based on the
policy specified by the PMA in the argument.

It would return 0 if successful. In all other cases it would return an
appropriate negative :c:macro:`errno` value.

Device Model with Power Management support
==========================================
Drivers that have Power Management support should initialize their devices
using the folllowing macros that are designed to take additional parameters
necessary to initialize the power management related handlers implemented by
the drivers.  Following are the macros:

:c:macro:`DEVICE_AND_API_INIT_PM`

    This should be used where :c:macro:`DEVICE_AND_API_INIT` macro would be
    used

:c:macro:`DEVICE_INIT_PM`

    This should be used where :c:macro:`DEVICE_INIT` macro would be used

:c:macro:`SYS_INIT_PM`

    This should be used where :c:macro:`SYS_INIT` macro would be used

Device PM API for PMA
=====================
These APIs will be used by the PMA to do suspend and resume operations on the
devices.

Getting the Device List
-----------------------
.. code-block:: c

   void device_list_get(struct device **device_list, int *device_count);

This api is used by the PMA to get the device list that the Zephyr kernel
internally maintains for all devices in the system.  The device structure in
the list will be used to identify devices that the PMA would chose to do PM
operations on.

The PMA can use this list to create its own sorted order list based on device
dependencies. It can also create device groups to execute different policies
on each device group.

.. note::
   PMA should take care that it does not alter the original list, since that is
   the list the kernel uses.

PMA Device Suspend API
----------------------
.. code-block:: c

   int device_suspend(struct device *device, int pm_policy);

This function would call the :c:func:`suspend()` handler function implemented
by the device driver for the device. Refer to
`Device Suspend Operation Handler function`_  for more information.

PMA Device Resume API
---------------------
.. code-block:: c

   int device_resume(struct device *device, int pm_policy);

This function would call the :c:func:`resume()` handler function implemented by
the device driver for the device. Refer to
`Device Resume Operation Handler function`_ for more information.

Device Busy Status Indication
=============================
Some power policies executed by the PMA could turn off power to devices causing
them to lose state.  If such devices are in the middle of some hardware
transaction (e.g. write to flash memory), when the power is turned off, then
such transactions would be left in an inconsistent state.  This infrastructure
is to enable devices guard such transactions by indicating to the PMA that it
is in the middle of a hardware transaction.

During a call to :c:func:`_sys_soc_suspend()`, if PMA finds any device is busy,
then it may decide to choose a policy other than Deep Sleep or defer PM
operations till the next call to :c:func:`_sys_soc_suspend()`.

.. note::
    It is up to the device driver writer to decide whether a transaction needs
    to be guarded using these APIs. If there are other recovery or retrial
    methods in place, then the driver could choose to avoid guarding the
    transactions.

Device API to Indicate Busy Status
----------------------------------
.. code-block:: c

   void device_busy_set(struct device *busy_dev);

This API will set a bit corresponding to the device, in a data structure
maintained by the kernel, to indicate it is in the middle of a transaction.

Device API to Clear Busy Status
-------------------------------
.. code-block:: c

   void device_busy_clear(struct device *busy_dev);

This API will clear a bit corresponding to the device, in a data structure
maintained by the kernel, to indicate it is not in the middle of a transaction.

PMA API to Check Busy Status of Single Device
---------------------------------------------
.. code-block:: c

   int device_busy_check(struct device *chk_dev);

This API is called by the PMA for each device to check its busy status. It
returns 0 if the device is not busy.

PMA API to Check Busy Status of All Devices
-------------------------------------------
.. code-block:: c

   int device_any_busy_check(void);

This API is called by the PMA to check if any device is busy. It returns 0 if
no device in the system is busy.

Power Management Config Flags
*****************************
The Power Management features can be individually enabled and disabled using
the following config flags.

`CONFIG_SYS_POWER_MANAGEMENT`
  This enables the Power Management Subsystem.

`CONFIG_SYS_POWER_LOW_POWER_STATE`
  PMA should enable this flag if it would use `SYS_PM_LOW_POWER_STATE`
  policy.

`CONFIG_SYS_POWER_DEEP_SLEEP`
  This flag would enable support for the `SYS_PM_DEEP_SLEEP` policy.

`CONFIG_DEVICE_POWER_MANAGEMENT`
  This flag should be enabled if device power management is supported by
  the PMA and the devices.

Writing a Power Management Application
**************************************
This section describes the steps involved in writing a typical Power Management
Application to enforce policies using the Power Management APIs. This
demonstrates the various scenarios that the PMA would consider in the policy
decision process.

.. note::

   The Power Management Application would be part of a larger application
   that does more than just power management. Here we refer to the Power
   Management part of the application.

Initial Setup
=============
To enable Power Management support, the application would do the following:

- Enable CONFIG_SYS_POWER_MANAGEMENT flag

- Enable other required config flags described in
  `Power Management Config Flags`_.

- Implement the hook functions described in
  `Power Management Hook Infrastructure`_.

Get Device List and Create Policies
===================================
The first act of the PMA will be to retrieve the known list of devices through
the :c:func:`device_list_get()` function.  Because the PMA is part of the
application, it is expected to start after all devices in the system have been
initialized. Thus the list of devices is not expected to change once the
application has begun.

The :c:func:`device_list_get()` function will return an array of current
enabled devices. It is up to the PMA to walk this list and determine the best
mechanism to store/process this list.  It is up to the system integrator to
verify the amount of time each device requires for a power cycle, and ensure
this time fits within the allotted time provided by the kernel.  This time
value is highly dependent upon each specific device used in the final platform
and SOC.

Once the device list has been retrieved and stored, the PMA can form device
groups and sorted lists based on device dependencies.  Using the device lists
and the known aggregate wake latency of the combination of power operations,
the PMA would then create fine grain custom power policies. Finally it would
map these custom policies to the policy categories defined by the Power
Management subsystem as described in `Policies`_.

Scenarios During Suspend
========================
When the :c:func:`_sys_soc_suspend()` function is called by the Power
Management subsystem, the PMA can select between multiple scenarios.

**Case 1:**

The time allotted is too short for any power management.  In this case, the PMA
will leave interrupts disabled, and return the code SYS_PM_NOT_HANDLED.  This
will allow the Zephyr kernel to continue with its normal idling process.

**Case 2:**

The time allotted is enough for some devices to be suspended.

.. code-block:: none

    Scan through the devices that meet the criteria

    Call device_suspend() for each device

    If everything suspends correctly, the PMA will:

        If the time allotted is enough for SYS_PM_LOW_POWER_STATE policy

            Setup wake event Push the CPU to LPS re-enabling interrupts at the
            same time.

            Return SYS_PM_LOW_POWER_STATE

        If the time allotted is not enough for SYS_PM_LOW_POWER_STATE

            Return SYS_PM_DEVICE_SUSPEND_ONLY

    If a device fails to suspend, the PMA will:

        If the device is not essential to the suspend process, as determined by
        the system integrator, the PMA can choose to ignore the failure.

        If the device is essential to the suspend process, as determined by the
        system integrator, the PMA shall take any necessary recovery actions and
        return SYS_PM_NOT_HANDLED.

**Case 3:**

The time allotted is enough for all devices to be suspended.

.. code-block:: none

    Call device_suspend() for each device.

    If everything suspends correctly, the PMA will:

        If the time allotted is enough for SYS_PM_DEEP_SLEEP policy

            Call device_any_busy_check() to get device busy status

            If any device is busy

                Choose policy other than SYS_PM_DEEP_SLEEP

            Setup wake event

            Push the SOC to Deep Sleep

            Re-enable interrupts

            Return SYS_PM_DEEP_SLEEP

        If the time allotted is enough for SYS_PM_LOW_POWER_STATE policy

            Setup wake event

            Push the CPU to LPS re-enabling interrupts at the same time.

            Return SYS_PM_LOW_POWER_STATE

        If the time allotted is not enough for any CPU or SOC operations

            Return SYS_PM_DEVICE_SUSPEND_ONLY

    If a device fails to suspend, the PMA will:

        If the device is not essential to the suspend process, as determined by
        the system integrator, the PMA can choose to ignore the failure.

        If the device is essential to the suspend process, as determined by the
        system integrator, the PMA shall take any necessary recovery actions and
        return SYS_PM_NOT_HANDLED.

Policy Decision Summary
=======================

    +------------------------------+---------------------------+
    | PM operations                | Policy and Return Code    |
    +==============================+===========================+
    | Suspend some devices &       | SYS_PM_LOW_POWER_STATE    |
    |                              |                           |
    | Enter Low Power State        |                           |
    +------------------------------+---------------------------+
    | Suspend all devices &        | SYS_PM_LOW_POWER_STATE    |
    |                              |                           |
    | Enter Low Power State        |                           |
    +------------------------------+---------------------------+
    | Suspend all devices &        | SYS_PM_DEEP_SLEEP         |
    |                              |                           |
    | Enter Deep Sleep             |                           |
    +------------------------------+---------------------------+
    | Suspend some or all devices &| SYS_PM_DEVICE_SUSPEND_ONLY|
    |                              |                           |
    | No CPU/SoC PM Operation      |                           |
    +------------------------------+---------------------------+
    | No PM operation              | SYS_PM_NOT_HANDLED        |
    +------------------------------+---------------------------+
