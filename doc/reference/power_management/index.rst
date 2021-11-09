.. _power_management_api:

Power Management
################

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

.. doxygengroup:: subsys_pm_sys_hooks

System Power Management APIs
============================

.. doxygengroup:: subsys_pm_sys

Device Power Management APIs
============================

.. doxygengroup:: subsys_pm_device

.. doxygengroup:: subsys_pm_device_runtime
