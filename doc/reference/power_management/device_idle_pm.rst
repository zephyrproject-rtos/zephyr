.. _device_idle_pm:

Device Idle Power Management
############################


The Device Idle Power Management framework is a Active Power
Management mechanism which reduces the overall system Power consumption
by suspending the devices which are idle or not being used while the
System is active or running.

This framework uses API's like device_set_power_state() and
device_get_power_state() to get/set the device power state. It
relies on device_busy_check API to know whether the device is
idle or busy before taking suspend or resume action.

The interfaces and APIs provided by the Device Idle PM are
designed to be generic and architecture independent.

Device Idle Power Management API
********************************

The Device Drivers use these APIs to perform device idle power management
operations on the devices.

Enable Device Idle Power Management of a Device API
===================================================

.. code-block:: c

   void device_pm_enable(struct device *dev);

Enbles Idle Power Management of the device.

Disable Device Idle Power Management of a Device API
====================================================

.. code-block:: c

   void device_pm_disable(struct device *dev);

Disables Idle Power Management of the device.

Resume Device asynchronously API
================================

.. code-block:: c

   int device_pm_get(struct device *dev);

Marks the device as being used. It will schedule a worker to asynchronously
bring the device to resume state if it is not already in active state. The API
returns 0 on success.

Resume Device synchronously API
===============================

.. code-block:: c

   int device_pm_get_sync(struct device *dev);

Marks the device as being used. It will bring up or resume the device
synchronously based on the device state and usage count. The API returns
0 on success.

Suspend Device asynchronously API
=================================

.. code-block:: c

   int device_pm_put(struct device *dev);

Marks the device as being free. It will schedule a worker to asynchronously
put the device to suspended state if it is not already in suspended. The API
returns 0 on success.

Suspend Device synchronously API
================================

.. code-block:: c

   int device_pm_put_sync(struct device *dev);

Marks the device as being free. It will suspend the device synchronously
based on the device state and usage count. The API returns 0 on success.

Device Idle PM Configuration Flags
**********************************

The Device Idle Power Management feature can be individually enabled and disabled
using the following configuration flag.

:code:`CONFIG_DEVICE_IDLE_PM`

   This flag enables the Device Idle Power Management.

API Reference
*************

Device Idle Power Management APIs
=================================

.. doxygengroup:: device_power_management_api
   :project: Zephyr

