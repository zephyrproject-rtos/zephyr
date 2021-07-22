.. _power_management_api:

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

:dfn:`Idle Thread`
   A system thread that runs when there are no other threads ready to run.

:dfn:`Power gating`
   Power gating reduces power consumption by shutting off current to blocks of
   the integrated circuit that are not in use.

:dfn:`Power State`
   SOC Power State describes processor and device power states implemented at
   the SOC level. Power states are represented by :c:enum:`pm_state` and each
   one has a different meaning.

:dfn:`Device Runtime Power Management`
   Device Runtime Power Management (PM) refers the capability of
   devices be able of saving energy independently of the the
   system. Devices will keep reference of their usage and will
   automatically be suspended or resumed. This feature is enabled via
   the ::kconfig:`CONFIG_PM_DEVICE_RUNTIME` Kconfig option.

Overview
********

The interfaces and APIs provided by the power management subsystem
are designed to be architecture and SOC independent. This enables power
management implementations to be easily adapted to different SOCs and
architectures.

The architecture and SOC independence is achieved by separating the core
infrastructure and the SOC specific implementations. The SOC specific
implementations are abstracted to the application and the OS using hardware
abstraction layers.

The power management features are classified into the following categories.

* Tickless Idle
* System Power Management
* Device Power Management

System Power Management
***********************

The kernel enters the idle state when it has nothing to schedule. If enabled via
the :kconfig:`CONFIG_PM` Kconfig option, the Power Management
Subsystem can put an idle system in one of the supported power states, based
on the selected power management policy and the duration of the idle time
allotted by the kernel.

It is an application responsibility to set up a wake up event. A wake up event
will typically be an interrupt triggered by one of the SoC peripheral modules
such as a SysTick, RTC, counter, or GPIO. Depending on the power mode entered,
only some SoC peripheral modules may be active and can be used as a wake up
source.

The following diagram describes system power management:

.. image:: system-pm.svg
   :align: center
   :alt: System power management

Some handful examples using different power management features:

* :zephyr_file:`samples/boards/stm32/power_mgmt/blinky/`
* :zephyr_file:`samples/boards/nrf/system_off/`
* :zephyr_file:`samples/subsys/pm/device_pm/`
* :zephyr_file:`tests/subsys/pm/power_mgmt/`
* :zephyr_file:`tests/subsys/pm/power_mgmt_soc/`
* :zephyr_file:`tests/subsys/pm/power_state_api/`

Power States
============

The power management subsystem contains a set of states based on
power consumption and context retention.

The list of available power states is defined by :c:enum:`pm_state`. In
general power states with higher indexes will offer greater power savings and
have higher wake latencies. Following is a thorough list of available states:

.. doxygenenumvalue:: PM_STATE_ACTIVE

.. doxygenenumvalue:: PM_STATE_RUNTIME_IDLE

.. doxygenenumvalue:: PM_STATE_SUSPEND_TO_IDLE

.. doxygenenumvalue:: PM_STATE_STANDBY

.. doxygenenumvalue:: PM_STATE_SUSPEND_TO_RAM

.. doxygenenumvalue:: PM_STATE_SUSPEND_TO_DISK

.. doxygenenumvalue:: PM_STATE_SOFT_OFF

.. _pm_constraints:

Power States Constraint
=======================

The power management subsystem allows different Zephyr components and
applications to set constraints on various power states preventing the
system to go these states. This can be used by devices when executing
tasks in background to avoid the system to go to state where it would
lose context. Constraints can be set, released and checked using the
follow APIs:

.. doxygenfunction:: pm_constraint_set

.. doxygenfunction:: pm_constraint_release

.. doxygenfunction:: pm_constraint_get

Power Management Policies
=========================

The power management subsystem supports the following power management policies:

* Residency
* Application
* Dummy

The policy manager is responsible to inform the power subsystem which
power state the system should go based on states available in the
platform and possible runtime :ref:`constraints<pm_constraints>`

Information about states can be get from device tree, see
:zephyr_file:`dts/bindings/power/state.yaml`.

Residency
---------

The power management system enters the power state which offers the highest
power savings, and with a minimum residency value (in device tree, see
:zephyr_file:`dts/bindings/power/state.yaml`) less than or equal to
the scheduled system idle time duration.

This policy also accounts for the time necessary to become active
again. The core logic used by this policy to select the best power
state is:

.. code-block:: c

   if (time_to_next_scheduled_event >= (state.min_residency_us + state.exit_latency))) {
      return state
   }

Application
-----------

The power management policy is defined by the application which has to implement
the following function.

.. code-block:: c

   struct pm_state_info pm_policy_next_state(int32_t ticks);

In this policy the application is free to decide which power state the
system should go based on the remaining time for the next scheduled
timeout.

An example of an application that defines its own policy can be found in
:zephyr_file:`tests/subsys/pm/power_mgmt/`.

Dummy
-----

This policy returns the next supported power state in a loop. It is used mainly
for testing purposes.

Device Power Management Infrastructure
**************************************

The device power management infrastructure consists of interfaces to the
Zephyr RTOS device model. These APIs send control commands to the device driver
to update its power state or to get its current power state.

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
SOC interface can enter CPU or SOC power states quickly when
:code:`pm_system_suspend()` gets called. This is because it does not need to
spend time doing device power management if the devices are already put in
the appropriate power state by the application or component managing the
devices.

Central method
==============

In this method device power management is mostly done inside
:code:`pm_system_suspend()` along with entering a CPU or SOC power state.

If a decision to enter deep sleep is made, the implementation would enter it
only after checking if the devices are not in the middle of a hardware
transaction that cannot be interrupted. This method can be used in
implementations where the applications and components using devices are not
expected to be power aware and do not implement device power management.

.. image:: central_method.svg
   :align: center

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

:code:`PM_DEVICE_STATE_ACTIVE`

   Normal operation of the device. All device context is retained.

:code:`PM_DEVICE_STATE_LOW_POWER`

   Device context is preserved by the HW and need not be restored by the driver.

:code:`PM_DEVICE_STATE_SUSPEND`

   Most device context is lost by the hardware. Device drivers must save and
   restore or reinitialize any context lost by the hardware.

:code:`PM_DEVICE_STATE_SUSPENDING`

   Device is currently transitioning from :c:macro:`PM_DEVICE_STATE_ACTIVE` to
   :c:macro:`PM_DEVICE_STATE_SUSPEND`.

:code:`PM_DEVICE_STATE_RESUMING`

   Device is currently transitioning from :c:macro:`PM_DEVICE_STATE_SUSPEND` to
   :c:macro:`PM_DEVICE_STATE_ACTIVE`.

:code:`PM_DEVICE_STATE_OFF`

   Power has been fully removed from the device. The device context is lost
   when this state is entered. Need to reinitialize the device when powering
   it back on.

Device Power Management Operations
==================================

Zephyr RTOS power management subsystem provides a control function interface
to device drivers to indicate power management operations to perform.
The supported PM control commands are:

* PM_DEVICE_STATE_SET
* PM_DEVICE_STATE_GET

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

Drivers initialize the devices using macros. See :ref:`device_model_api` for
details on how these macros are used. Use the DEVICE_DEFINE macro to initialize
drivers providing power management support via the PM control function.
One of the macro parameters is the pointer to the pm_control handler function.
If the driver doesn't implement any power control operations, it can initialize
the corresponding pointer with ``NULL``.

Device Power Management API
===========================

The SOC interface and application use these APIs to perform power management
operations on the devices.

Get Device List
---------------

.. code-block:: c

   size_t z_device_get_all_static(struct device const **device_list);

The Zephyr RTOS kernel internally maintains a list of all devices in the system.
The SOC interface uses this API to get the device list. The SOC interface can use the list to
identify the devices on which to execute power management operations.

.. note::

   Ensure that the SOC interface does not alter the original list. Since the kernel
   uses the original list, it must remain unchanged.

Device Set Power State
----------------------

.. code-block:: c

   int pm_device_state_set(const struct device *dev, uint32_t device_power_state, pm_device_cb cb, void *arg);

Calls the :c:func:`pm_control()` handler function implemented by the
device driver with PM_DEVICE_STATE_SET command.

Device Get Power State
----------------------

.. code-block:: c

   int pm_device_state_get(const struct device *dev, uint32_t * device_power_state);

Calls the :c:func:`pm_control()` handler function implemented by the
device driver with PM_DEVICE_STATE_GET command.

Busy Status Indication
======================

The SOC interface executes some power policies that can turn off power to devices,
causing them to lose their state. If the devices are in the middle of some
hardware transaction, like writing to flash memory when the power is turned
off, then such transactions would be left in an inconsistent state. This
infrastructure guards such transactions by indicating to the SOC interface that
the device is in the middle of a hardware transaction.

When the :c:func:`pm_system_suspend()` is called, depending on the power state
returned by the policy manager, the system may suspend or put devices in low
power if they are not marked as busy.

Here are the APIs used to set, clear, and check the busy status of devices.

Indicate Busy Status API
------------------------

.. code-block:: c

   void device_busy_set(const struct device *busy_dev);

Sets a bit corresponding to the device, in a data structure maintained by the
kernel, to indicate whether or not it is in the middle of a transaction.

Clear Busy Status API
---------------------

.. code-block:: c

   void device_busy_clear(const struct device *busy_dev);

Clears the bit corresponding to the device in a data structure
maintained by the kernel to indicate that the device is not in the middle of
a transaction.

Check Busy Status of Single Device API
--------------------------------------

.. code-block:: c

   int device_busy_check(const struct device *chk_dev);

Checks whether a device is busy. The API returns 0 if the device
is not busy.

This API is used by the system power management.

Check Busy Status of All Devices API
------------------------------------

.. code-block:: c

   int device_any_busy_check(void);

Checks if any device is busy. The API returns 0 if no device in the system is busy.

Device Runtime Power Management
*******************************

The Device Runtime Power Management framework is an Active Power
Management mechanism which reduces the overall system Power consumtion
by suspending the devices which are idle or not being used while the
System is active or running.

The framework uses :c:func:`pm_device_state_set()` API set the
device power state accordingly based on the usage count.

The interfaces and APIs provided by the Device Runtime PM are
designed to be generic and architecture independent.

Device Runtime Power Management API
===================================

The Device Drivers use these APIs to perform device runtime power
management operations on the devices.

Enable Device Runtime Power Management of a Device API
------------------------------------------------------

.. code-block:: c

   void pm_device_enable(const struct device *dev);

Enables Runtime Power Management of the device.

Disable Device Runtime Power Management of a Device API
-------------------------------------------------------

.. code-block:: c

   void pm_device_disable(const struct device *dev);

Disables Runtime Power Management of the device.

Resume Device asynchronously API
--------------------------------

.. code-block:: c

   int pm_device_get_async(const struct device *dev);

Marks the device as being used. This API will asynchronously
bring the device to resume state if it was suspended. If the device
was already active, it just increments the device usage count.
The API returns 0 on success.

Device drivers can monitor this operation to finish calling
:c:func:`pm_device_wait`.

Resume Device synchronously API
-------------------------------

.. code-block:: c

   int pm_device_get(const struct device *dev);

Marks the device as being used. It will bring up or resume
the device if it is in suspended state based on the device
usage count. This call is blocked until the device PM state
is changed to active. The API returns 0 on success.

Suspend Device asynchronously API
---------------------------------

.. code-block:: c

   int pm_device_put_async(const struct device *dev);

Releases a device. This API asynchronously puts the device to suspend
state if not already in suspend state if the usage count of this device
reaches 0.

Device drivers can monitor this operation to finish calling
:c:func:`pm_device_wait`.

Suspend Device synchronously API
--------------------------------

.. code-block:: c

   int pm_device_put(const struct device *dev);

Marks the device as being released. It will put the device to
suspended state if is is in active state based on the device
usage count. This call is blocked until the device PM state
is changed to resume. The API returns 0 on success. This
call is blocked until the device is suspended.


Power Management Configuration Flags
************************************

The Power Management features can be individually enabled and disabled using
the following configuration flags.

:kconfig:`CONFIG_PM`

   This flag enables the power management subsystem.

:kconfig:`CONFIG_PM_DEVICE`

   This flag is enabled if the SOC interface and the devices support device power
   management.

:kconfig:`CONFIG_PM_DEVICE_RUNTIME`

   This flag enables the Runtime Power Management.

API Reference
*************

Power Management Hook Interface
===============================

.. doxygengroup:: power_management_hook_interface

System Power Management APIs
============================

.. doxygengroup:: system_power_management_api

Device Power Management APIs
============================

.. doxygengroup:: device_power_management_api
