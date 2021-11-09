Device Runtime Power Management
###############################

The Device Runtime Power Management framework is an Active Power
Management mechanism which reduces the overall system Power consumtion
by suspending the devices which are idle or not being used while the
System is active or running.

The framework uses :c:func:`pm_device_state_set()` API set the
device power state accordingly based on the usage count.

The interfaces and APIs provided by the Device Runtime PM are
designed to be generic and architecture independent.

Device Runtime Power Management API
***********************************

The Device Drivers use these APIs to perform device runtime power
management operations on the devices.

Enable Device Runtime Power Management of a Device API
======================================================

.. code-block:: c

   void pm_device_runtime_enable(const struct device *dev);

Enables Runtime Power Management of the device.

Disable Device Runtime Power Management of a Device API
=======================================================

.. code-block:: c

   void pm_device_runtime_disable(const struct device *dev);

Disables Runtime Power Management of the device.

Resume Device synchronously API
===============================

.. code-block:: c

   int pm_device_runtime_get(const struct device *dev);

Marks the device as being used. It will bring up or resume
the device if it is in suspended state based on the device
usage count. This call is blocked until the device PM state
is changed to active. The API returns 0 on success.

Suspend Device asynchronously API
=================================

.. code-block:: c

   int pm_device_runtime_put_async(const struct device *dev);

Releases a device. This API asynchronously puts the device to suspend
state if not already in suspend state if the usage count of this device
reaches 0.

Suspend Device synchronously API
================================

.. code-block:: c

   int pm_device_runtime_put(const struct device *dev);

Marks the device as being released. It will put the device to
suspended state if is is in active state based on the device
usage count. This call is blocked until the device PM state
is changed to resume. The API returns 0 on success. This
call is blocked until the device is suspended.
