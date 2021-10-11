Power Domain
############

Introduction
************

Most of the devices in an SoC have independent power control that can
be turned on or off to reduce power consumption. But there is a
significant amount of static current leakage that can't be controlled
only using device power management. To solve this problem, SoCs
normally are divided into several regions grouping devices that
are generally used together, so that an unused region can be
completely powered off to eliminate this leakage. These regions are
called "power domains", can be present in a hierarchy and can be
nested.

Power domains are optional on Zephyr, to enable this feature the
option :kconfig:`PM_DEVICE_POWER_DOMAIN` has to be set. When it is
enabled any device can be used as power domain just declaring itself
in devicetree compatible with :code:`power-domain` in
``compatible`` node's property.

When a power domain becomes active, suspended or is turned off, all devices
using it have their power management callback called with the
:c:enumerator:`PM_DEVICE_ACTION_HANDLE_PD_RESUME` or
:c:enumerator:`PM_DEVICE_ACTION_HANDLE_PD_SUSPEND` or
:c:enumerator:`PM_DEVICE_ACTION_HANDLE_PD_OFF` respectively. This
work flow is illustrated in the diagram bellow.

.. _pm-domain-work-flow:

.. graphviz::
   :caption: Power domain work flow

   digraph {
       rankdir="TB";

       action [style=invis]
       {
           rank = same;
           rankdir="LR"
           devA [label="gpio0"]
           devB [label="gpio1"]
       }
       domain [label="gpio_domain"]

      action -> devA [label="pm_device_get()"]
      devA:se -> domain:n [label="pm_device_get()"]

      domain -> devB [label="action_cb(PM_DEVICE_ACTION_HANDLE_PD_RESUME)"]
      domain:sw -> devA:sw [label="action_cb(PM_DEVICE_ACTION_HANDLE_PD_RESUME)"]
   }

Implementation guidelines
*************************

In a first place, a device that acts as a power domain needs to
declare compatible with ``power-domain``. Taking
:ref:`pm-domain-work-flow` as example, the following code defines a
domain called ``gpio_domain``.

.. code-block:: devicetree

	gpio_domain: gpio_domain@4 {
		compatible = "power-domain";
		...
	};

A power domain needs to implement the PM action callback used by the
PM subsystem to resume, suspend and turn off devices.

.. code-block:: c

    static int mydomain_pm_action(const struct device *dev,
                               enum pm_device_action *action)
    {
        switch (action) {
        case PM_DEVICE_ACTION_SUSPEND:
            /* suspend the domain */
            ...
            break;
        case PM_DEVICE_ACTION_TURN_OFF:
            /* turn off the domain */
            ...
            break;
        case PM_DEVICE_ACTION_RESUME:
            /* resume the domain */
            ...
            break;
        default:
            return -ENOTSUP;
        }

        return 0;
    }

Devices belonging to this device can be declared referring it in the
``power-domain`` node's property. The example below declares devices
``gpio0`` and ``gpio1`` belonging to domain ``gpio_domain```.

.. code-block:: devicetree

        &gpio0 {
                compatible = "zephyr,gpio-emul";
                gpio-controller;
                power-domain = <&gpio_domain>;
        };

        &gpio1 {
                compatible = "zephyr,gpio-emul";
                gpio-controller;
                power-domain = <&gpio_domain>;
        };

All devices under a domain will be notified when the domain changes
state. These notifications are sent as actions in the device PM action
callback and can be used by them to do any additional work required.
They can safely be ignored though.

.. code-block:: c

    static int mydev_pm_action(const struct device *dev,
                               enum pm_device_action *action)
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
        case PM_DEVICE_ACTION_HANDLE_PD_RESUME
            /* domain it belongs to was resumed */
            ...
            break;
        case PM_DEVICE_ACTION_HANDLE_PD_SUSPEND
            /* domain it belongs to will be suspended */
            ...
            break;
        case PM_DEVICE_ACTION_HANDLE_PD_OFF
            /* domain it belongs to will be turned off */
            ...
            break;
        default:
            return -ENOTSUP;
        }

        return 0;
    }

.. note::

   Devices will receive notifications about power domain state changes
   independently of their states.

.. note::

   It is responsibility of driver or the application to set the domain as
   "wakeup" source if a device depending on it is used as "wakeup" source.
