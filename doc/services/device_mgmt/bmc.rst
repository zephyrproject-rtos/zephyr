.. _bmc:

Baseboard Management Controller
###############################

.. contents::
    :local:
    :depth: 2

Overview
********

A Baseboard Management Controller (BMC) is the service processor of a computer
system. It powers the host on and off, watches its sensors, captures its
console and exposes all of that to remote management clients, independently of
whether the host itself is running.

The BMC subsystem, enabled with :kconfig:option:`CONFIG_BMC`, provides the
generic half of such a controller:

* HTTP and HTTPS services, and a `Redfish`_ service on top of them
* an administrator account and pluggable authentication
* configuration storage on the :ref:`settings_api` subsystem
* host power control, reset and status LED
* a sensor registry surfaced over Redfish and the shell
* host console capture, with a websocket bridge to a browser terminal
* an authenticated websocket for the Zephyr shell, when
  :kconfig:option:`CONFIG_SHELL_BACKEND_WEBSOCKET` is enabled
* vital product data, NTP and RTC synchronisation, DHCPv4 and a JTAG bridge

Everything that is specific to a product -- its name, its dashboard, its
sensors, how it reaches its host -- is left to the application. A complete
reference application is provided in :zephyr:code-sample:`bmc`, and is meant to
be copied and adapted.

.. _Redfish: https://www.dmtf.org/standards/redfish

Starting the BMC
****************

The application starts the BMC with a single call:

.. code-block:: c

   #include <zephyr/mgmt/bmc.h>

   int main(void)
   {
           return bmc_init();
   }

:c:func:`bmc_init` walks the registered components phase by phase, in the order
of :c:enum:`bmc_init_phase`: storage, platform, network, services and finally
the application. A component that fails aborts the boot unless it was declared
optional, in which case the failure is logged and the boot continues.

Every module of the subsystem is registered this way, and so is anything the
application adds:

.. code-block:: c

   static int my_component_init(void)
   {
           return 0;
   }

   BMC_COMPONENT_DEFINE(my_component, BMC_INIT_PHASE_APP, my_component_init, false);

Extension points
****************

Host control
============

The host is driven through a :c:struct:`bmc_host_ops`. With
:kconfig:option:`CONFIG_BMC_HOST_GPIO` the subsystem installs a backend that
toggles the ``bmc-host-power``, ``bmc-host-power-2``, ``bmc-host-reset`` and
``bmc-status-led`` devicetree aliases. A product whose host is reached over a
mailbox, an eSPI link or another service processor registers its own backend
instead:

.. code-block:: c

   static const struct bmc_host_ops my_host_ops = {
           .power_set = my_power_set,
           .power_get = my_power_get,
           .reset = my_reset,
   };

   static int my_host_init(void)
   {
           return bmc_host_ops_register(&my_host_ops);
   }

   BMC_COMPONENT_DEFINE(my_host, BMC_INIT_PHASE_PLATFORM, my_host_init, false);

Sensors
=======

Sensors are registered at build time. A sensor backed by a Zephyr sensor
device only needs its devicetree node and the metadata Redfish reports:

.. code-block:: c

   BMC_SENSOR_DT_DEFINE(die_temp, DT_ALIAS(bmc_die_temp), SENSOR_CHAN_DIE_TEMP,
                        "TempBmc", "BMC Die Temperature", "Temperature", "Cel");

Anything else supplies a read callback with ``BMC_SENSOR_DEFINE()``. Every
registered sensor appears in the Redfish sensor collection and under
``bmc sensor`` in the shell.

Redfish
=======

The subsystem serves the standard resources and holds no product strings of its
own; they come from a :c:struct:`bmc_redfish_identity`, which defaults to the
``CONFIG_BMC_REDFISH_*`` Kconfig values:

.. code-block:: c

   static const struct bmc_redfish_identity identity = {
           .product_name = "Example Board",
           .manufacturer = "Example Inc",
           .model = "EX-1",
   };

   bmc_redfish_identity_register(&identity);

``BMC_REDFISH_OEM_DEFINE()`` appends vendor members to a standard resource, and
``BMC_REDFISH_RESOURCE_DEFINE()`` publishes a resource of its own:

.. code-block:: c

   BMC_REDFISH_RESOURCE_DEFINE(my_oem, "/redfish/v1/Oem/Example", true,
                               my_oem_get, NULL, NULL);

All ``/redfish`` requests are routed by the subsystem to the resource whose URL
matches, so handlers only deal with the request payload and the response body.

HTTP resources
==============

``BMC_HTTP_RESOURCE_DEFINE()`` attaches a resource to both the HTTP and the
HTTPS service; ``bmc_http_service`` and ``bmc_https_service`` can also be used
directly with :c:macro:`HTTP_RESOURCE_DEFINE`. This is how an application
serves its dashboard.

Events
======

:c:func:`bmc_callback_register` reports boot completion, network readiness,
configuration changes and host power transitions to the application.

Authentication
==============

By default the subsystem checks credentials against the administrator account
in its configuration store. Registering a :c:struct:`bmc_auth_ops` with
:c:func:`bmc_auth_ops_register` replaces that with, for example, a check
against a secure element or a remote directory.

.. warning::

   :kconfig:option:`CONFIG_BMC_DEFAULT_ADMIN_PASSWORD` has a placeholder value
   that the build warns about, and the BMC logs a warning at every boot for as
   long as the administrator still has that password. Set it, change the
   password at first boot, or register an authentication backend of your own.

Configuration storage
*********************

The BMC keeps its configuration under the ``bmc/`` tree of the
:ref:`settings_api` subsystem, so it works with any settings backend. An
application adds its own keys with its own settings handler. Without
:kconfig:option:`CONFIG_BMC_SETTINGS` the configuration lives in RAM and falls
back to the Kconfig defaults after every reboot.

API Reference
*************

.. doxygengroup:: bmc_api
