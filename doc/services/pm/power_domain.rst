.. _pm-power-domain:

Power Domain
############

Introduction
************

The Zephyr power domain abstraction is designed to support groupings of devices
powered by a common source to be notified of power source state changes in a
generic fashion. Application code that is using device A does not need to know
that device B is on the same power domain and should also be configured into a
low power state.

Power domains are optional on Zephyr, to enable this feature the
option :kconfig:option:`CONFIG_PM_DEVICE_POWER_DOMAIN` has to be set.

When a power domain turns itself on or off, it is the responsibility of the
power domain to notify all devices using it through their power management
callback called with
:c:enumerator:`PM_DEVICE_ACTION_TURN_ON` or
:c:enumerator:`PM_DEVICE_ACTION_TURN_OFF` respectively. This
work flow is illustrated in the diagram below.

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

      domain -> devB [label="action_cb(PM_DEVICE_ACTION_TURN_ON)"]
      domain:sw -> devA:sw [label="action_cb(PM_DEVICE_ACTION_TURN_ON)"]
   }

Internal Power Domains
----------------------

Most of the devices in an SoC have independent power control that can
be turned on or off to reduce power consumption. But there is a
significant amount of static current leakage that can't be controlled
only using device power management. To solve this problem, SoCs
normally are divided into several regions grouping devices that
are generally used together, so that an unused region can be
completely powered off to eliminate this leakage. These regions are
called "power domains", can be present in a hierarchy and can be
nested.

External Power Domains
----------------------

Devices external to a SoC can be powered from sources other than the main power
source of the SoC. These external sources are typically a switch, a regulator,
or a dedicated power IC. Multiple devices can be powered from the same source,
and this grouping of devices is typically called a "power domain".

Placing devices on power domains can be done for a variety of reasons,
including to enable devices with high power consumption in low power mode to be
completely turned off when not in use.

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
PM subsystem to turn devices on and off.

.. code-block:: c

    static int mydomain_pm_action(const struct device *dev,
                               enum pm_device_action *action)
    {
        switch (action) {
        case PM_DEVICE_ACTION_RESUME:
            /* resume the domain */
            ...
            /* notify children domain is now powered */
            pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_ON, NULL);
            break;
        case PM_DEVICE_ACTION_SUSPEND:
            /* notify children domain is going down */
            pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_OFF, NULL);
            /* suspend the domain */
            ...
            break;
        case PM_DEVICE_ACTION_TURN_ON:
            /* turn on the domain (e.g. setup control pins to disabled) */
            ...
            break;
        case PM_DEVICE_ACTION_TURN_OFF:
            /* turn off the domain (e.g. reset control pins to default state) */
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
        case PM_DEVICE_ACTION_TURN_ON:
            /* configure the device into low power mode */
            ...
            break;
        case PM_DEVICE_ACTION_TURN_OFF:
            /* prepare the device for power down */
            ...
            break;
        default:
            return -ENOTSUP;
        }

        return 0;
    }

.. note::

   It is responsibility of driver or the application to set the domain as
   "wakeup" source if a device depending on it is used as "wakeup" source.
