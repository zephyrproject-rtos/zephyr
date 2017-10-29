.. _power_management:

Power Management
################

Zephyr RTOS power management subsystem provides several means for a system
integrator to implement power management support that can take full
advantage of the power saving features of SOCs.


Terminology
***********

:dfn:`SOC interface`
   This is a general term for the components that have knowledge of the
   SOC and provide interfaces to the hardware features. It will abstract
   the SOC specific implementations to the applications and the OS.

:dfn:`CPU LPS (Low Power State)`
   Refers to any one of the low power states supported by the CPU. The CPU is
   usually powered on while the clocks are power gated.

:dfn:`Active State`
   The CPU and clocks are powered on. This is the normal operating state when
   the system is running.

:dfn:`Deep Sleep State`
   The CPU is power gated and loses context. Most peripherals would also be
   power gated. RAM is selectively retained.

:dfn:`SOC Power State`
   SOC Power State describes processor and device power states implemented at
   the SOC level. Deep Sleep State is an example of SOC Power State.

:dfn:`Idle Thread`
   A system thread that runs when there are no other threads ready to run.

:dfn:`Power gating`
   Power gating reduces power consumption by shutting off current to blocks of
   the integrated circuit that are not in use.

Overview
********

The interfaces and APIs provided by the power management subsystem
are designed to be architecture and SOC independent. This enables power
management implementations to be easily adapted to different SOCs and
architectures. The kernel does not implement any power schemes of its own, giving
the system integrator the flexibility of implementing custom power schemes.

The architecture and SOC independence is achieved by separating the core
infrastructure and the SOC specific implementations. The SOC specific
implementations are abstracted to the application and the OS using hardware
abstraction layers.

The power management features are classified into the following categories.

* Tickless Idle
* System Power Management
* Device Power Management

Tickless Idle
*************

This is the name used to identify the event-based idling mechanism of the
Zephyr RTOS kernel scheduler. The kernel scheduler can run in two modes. During
normal operation, when at least one thread is active, it sets up the system
timer in periodic mode and runs in an interval-based scheduling mode. The
interval-based mode allows it to time slice between threads. Many times, the
threads would be waiting on semaphores, timeouts or for events. When there
are no threads running, it is inefficient for the kernel scheduler to run
in interval-based mode. This is because, in this mode the timer would trigger
an interrupt at fixed intervals causing the scheduler to be invoked at each
interval. The scheduler checks if any thread is ready to run. If no thread
is ready to run then it is a waste of power because of the unnecessary CPU
processing. This is avoided by the kernel switching to event-based idling
mode whenever there is no thread ready to run.

The kernel holds an ordered list of thread timeouts in the system. These are
the amount of time each thread has requested to wait. When the last active
thread goes to wait, the idle thread is scheduled. The idle thread programs
the timer to one-shot mode and programs the count to the earliest timeout
from the ordered thread timeout list. When the timer expires, a timer event
is generated. The ISR of this event will invoke the scheduler, which would
schedule the thread associated with the timeout. Before scheduling the
thread, the scheduler would switch the timer again to periodic mode. This
method saves power because the CPU is removed from the wait only when there
is a thread ready to run or if an external event occurred.

System Power Management
***********************

This consists of the hook functions that the power management subsystem calls
when the kernel enters and exits the idle state, in other words, when the kernel
has nothing to schedule. This section provides a general overview of the hook
functions. Refer to :ref:`power_management_api` for the detailed description of
the APIs.

Suspend Hook function
=====================

.. code-block:: c

   int _sys_soc_suspend(s32_t ticks);

When the kernel is about to go idle, the power management subsystem calls the
:code:`_sys_soc_suspend()` function, notifying the SOC interface that the kernel
is ready to enter the idle state.

At this point, the kernel has disabled interrupts and computed the maximum
time the system can remain idle. The function passes the time that
the system can remain idle. The SOC interface performs power operations that
can be done in the available time. The power management operation must halt
execution on a CPU or SOC low power state. Before entering the low power state,
the SOC interface must setup a wake event.

The power management subsystem expects the :code:`_sys_soc_suspend()` to
return one of the following values based on the power management operations
the SOC interface executed:

:code:`SYS_PM_NOT_HANDLED`

   Indicates that no power management operations were performed.

:code:`SYS_PM_LOW_POWER_STATE`

   Indicates that the CPU was put in a low power state.

:code:`SYS_PM_DEEP_SLEEP`

   Indicates that the SOC was put in a deep sleep state.

Resume Hook function
====================

.. code-block:: c

   void _sys_soc_resume(void);

The power management subsystem optionally calls this hook function when exiting
kernel idling if power management operations were performed in
:code:`_sys_soc_suspend()`. Any necessary recovery operations can be performed
in this function before the kernel scheduler schedules another thread. Some
power states may not need this notification. It can be disabled by calling
:code:`_sys_soc_pm_idle_exit_notification_disable()` from
:code:`_sys_soc_suspend()`.

Resume From Deep Sleep Hook function
====================================

.. code-block:: c

   void _sys_soc_resume_from_deep_sleep(void);

This function is optionally called when exiting from deep sleep if the SOC
interface does not have bootloader support to handle resume from deep sleep.
This function should restore context to the point where system entered
the deep sleep state.

.. note::

   Since the hook functions are called with the interrupts disabled, the SOC
   interface should ensure that its operations are completed quickly. Thus, the
   SOC interface ensures that the kernel's scheduling performance is not
   disrupted.

Power Schemes
*************

When the power management subsystem notifies the SOC interface that the kernel
is about to enter a system idle state, it specifies the period of time the
system intends to stay idle. The SOC interface can perform various power
management operations during this time. For example, put the processor or the
SOC in a low power state, turn off some or all of the peripherals or power gate
device clocks.

Different levels of power savings and different wake latencies characterize
these power schemes. In general, operations that save more power have a
higher wake latency. When making decisions, the SOC interface chooses the
scheme that saves the most power. At the same time, the scheme's total
execution time must fit within the idle time allotted by the power management
subsystem.

The power management subsystem classifies power management schemes
into two categories based on whether the CPU loses execution context during the
power state transition.

* SYS_PM_LOW_POWER_STATE
* SYS_PM_DEEP_SLEEP

SYS_PM_LOW_POWER_STATE
======================

CPU does not lose execution context. Devices also do not lose power while
entering power states in this category. The wake latencies of power states
in this category are relatively low.

SYS_PM_DEEP_SLEEP
=================

CPU is power gated and loses execution context. Execution will resume at
OS startup code or at a resume point determined by a bootloader that supports
deep sleep resume. Depending on the SOC's implementation of the power saving
feature, it may turn off power to most devices. RAM may be retained by some
implementations, while others may remove power from RAM saving considerable
power. Power states in this category save more power than
`SYS_PM_LOW_POWER_STATE`_ and would have higher wake latencies.

Device Power Management Infrastructure
**************************************

The device power management infrastructure consists of interfaces to the
Zephyr RTOS device model. These APIs send control commands to the device driver
to update its power state or to get its current power state.
Refer to  :ref:`power_management_api` for detailed descriptions of the APIs.

Zephyr RTOS supports two methods of doing device power management.

* Distributed method
* Central method

Distributed method
==================

In this method, the application or any component that deals with devices directly
and has the best knowledge of their use does the device power management. This
saves power if some devices that are not in use can be turned off or put
in power saving mode. This method allows saving power even when the CPU is
active. The components that use the devices need to be power aware and should
be able to make decisions related to managing device power. In this method, the
SOC interface can enter CPU or SOC low power states quickly when
:code:`_sys_soc_suspend()` gets called. This is because it does not need to
spend time doing device power management if the devices are already put in
the appropriate low power state by the application or component managing the
devices.

Central method
==============

In this method device power management is mostly done inside
:code:`_sys_soc_suspend()` along with entering a CPU or SOC low power state.

If a decision to enter deep sleep is made, the implementation would enter it
only after checking if the devices are not in the middle of a hardware
transaction that cannot be interrupted. This method can be used in
implementations where the applications and components using devices are not
expected to be power aware and do not implement device power management.

This method can also be used to emulate a hardware feature supported by some
SOCs which cause automatic entry to deep sleep when all devices are idle.
Refer to `Busy Status Indication`_ to see how to indicate whether a device is busy
or idle.

Device Power Management States
==============================
The Zephyr RTOS power management subsystem defines four device states.
These states are classified based on the degree of device context that gets lost
in those states, kind of operations done to save power, and the impact on the
device behavior due to the state transition. Device context includes device
registers, clocks, memory etc.

The four device power states:

:code:`DEVICE_PM_ACTIVE_STATE`

   Normal operation of the device. All device context is retained.

:code:`DEVICE_PM_LOW_POWER_STATE`

   Device context is preserved by the HW and need not be restored by the driver.

:code:`DEVICE_PM_SUSPEND_STATE`

   Most device context is lost by the hardware. Device drivers must save and
   restore or reinitialize any context lost by the hardware.

:code:`DEVICE_PM_OFF_STATE`

   Power has been fully removed from the device. The device context is lost
   when this state is entered. Need to reinitialize the device when powering
   it back on.

Device Power Management Operations
==================================

Zephyr RTOS power management subsystem provides a control function interface
to device drivers to indicate power management operations to perform.
The supported PM control commands are:

* DEVICE_PM_SET_POWER_STATE
* DEVICE_PM_GET_POWER_STATE

Each device driver defines:

* The device's supported power states.
* The device's supported transitions between power states.
* The device's necessary operations to handle the transition between power states.

The following are some examples of operations that the device driver may perform
in transition between power states:

* Save/Restore device states.
* Gate/Un-gate clocks.
* Gate/Un-gate power.
* Mask/Un-mask interrupts.

Device Model with Power Management Support
==========================================

Drivers initialize the devices using macros. See :ref:`device_drivers` for
details on how these macros are used. Use the DEVICE_DEFINE macro to initialize
drivers providing power management support via the PM control function.
One of the macro parameters is the pointer to the device_pm_control handler function.

Default Initializer Function
----------------------------

.. code-block:: c

   int device_pm_control_nop(struct device *unused_device, u32_t unused_ctrl_command, void *unused_context);


If the driver doesn't implement any power control operations, the driver can
initialize the corresponding pointer with this default nop function. This
default nop function does nothing and should be used instead of
implementing a dummy function to avoid wasting code memory in the driver.


Device Power Management API
===========================

The SOC interface and application use these APIs to perform power management
operations on the devices.

Get Device List
---------------

.. code-block:: c

   void device_list_get(struct device **device_list, int *device_count);

The Zephyr RTOS kernel internally maintains a list of all devices in the system.
The SOC interface uses this API to get the device list. The SOC interface can use the list to
identify the devices on which to execute power management operations.

.. note::

   Ensure that the SOC interface does not alter the original list. Since the kernel
   uses the original list, it must remain unchanged.

Device Set Power State
----------------------

.. code-block:: c

   int device_set_power_state(struct device *device, u32_t device_power_state);

Calls the :c:func:`device_pm_control()` handler function implemented by the
device driver with DEVICE_PM_SET_POWER_STATE command.

Device Get Power State
----------------------

.. code-block:: c

   int device_get_power_state(struct device *device, u32_t * device_power_state);

Calls the :c:func:`device_pm_control()` handler function implemented by the
device driver with DEVICE_PM_GET_POWER_STATE command.

Busy Status Indication
======================

The SOC interface executes some power policies that can turn off power to devices,
causing them to lose their state. If the devices are in the middle of some
hardware transaction, like writing to flash memory when the power is turned
off, then such transactions would be left in an inconsistent state. This
infrastructure guards such transactions by indicating to the SOC interface that
the device is in the middle of a hardware transaction.

When the :code:`_sys_soc_suspend()` is called, the SOC interface checks if any device
is busy. The SOC interface can then decide to execute a power management scheme other than deep sleep or
to defer power management operations until the next call of
:code:`_sys_soc_suspend()`.

An alternative to using the busy status mechanism is to use the
`distributed method`_ of device power management. In such a method where the
device power management is handled in a distributed manner rather than centrally in
:code:`_sys_soc_suspend()`, the decision to enter deep sleep can be made based
on whether all devices are already turned off.

This feature can be also used to emulate a hardware feature found in some SOCs
that causes the system to automatically enter deep sleep when all devices are idle.
In such an usage, the busy status can be set by default and cleared as each
device becomes idle. When :code:`_sys_soc_suspend()` is called, deep sleep can
be entered if no device is found to be busy.

Here are the APIs used to set, clear, and check the busy status of devices.

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

Power Management Configuration Flags
************************************

The Power Management features can be individually enabled and disabled using
the following configuration flags.

:code:`CONFIG_SYS_POWER_MANAGEMENT`

   This flag enables the power management subsystem.

:code:`CONFIG_TICKLESS_IDLE`

   This flag enables the tickless idle power saving feature.

:code:`CONFIG_SYS_POWER_LOW_POWER_STATE`

   The SOC interface enables this flag to use the :code:`SYS_PM_LOW_POWER_STATE` policy.

:code:`CONFIG_SYS_POWER_DEEP_SLEEP`

   This flag enables support for the :code:`SYS_PM_DEEP_SLEEP` policy.

:code:`CONFIG_DEVICE_POWER_MANAGEMENT`

   This flag is enabled if the SOC interface and the devices support device power
   management.

