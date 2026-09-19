.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:code-sample:: bmc
   :name: Baseboard Management Controller

   Build a product BMC on top of the generic BMC subsystem.

Overview
********

This sample is a complete, copyable Baseboard Management Controller product
built on :file:`subsys/mgmt/bmc`. The subsystem provides the mechanism -- the
HTTP and Redfish services, the configuration store, host control, the sensor
registry and the console bridge -- while everything that makes a BMC belong to
a particular board lives here, in the application:

.. list-table::
   :header-rows: 1

   * - File
     - Responsibility
   * - :file:`src/main.c`
     - Calls :c:func:`bmc_init`.
   * - :file:`src/board.c`
     - Publishes the board sensors and, on boards without host GPIOs,
       registers a host control backend.
   * - :file:`src/redfish_product.c`
     - Registers the product identity, an OEM extension on the Manager
       resource and an extra Redfish resource.
   * - :file:`src/webui.c` and :file:`web/`
     - The dashboard, its branding and the ``/webui/features`` endpoint.
   * - :file:`src/tls.c` and :file:`certs/`
     - Server certificate for the HTTPS service.

Copy the directory, replace those files, and the BMC core needs no patching.

Extending the BMC
*****************

The application hooks into the core through the headers under
:file:`include/zephyr/mgmt/bmc/`.

**Components.** Anything the application wants started as part of the BMC boot
is registered as a component and runs in the phase it belongs to, so the
application never has to edit the core's boot sequence:

.. code-block:: c

   static int board_host_init(void)
   {
           return bmc_host_ops_register(&my_host_ops);
   }

   BMC_COMPONENT_DEFINE(my_host, BMC_INIT_PHASE_PLATFORM, board_host_init, false);

**Host control.** A :c:struct:`bmc_host_ops` registered with
:c:func:`bmc_host_ops_register` replaces the GPIO backend selected by
:kconfig:option:`CONFIG_BMC_HOST_GPIO`, which is how a product that reaches its
host over a mailbox, an eSPI link or a service processor plugs in.

**Sensors.** ``BMC_SENSOR_DT_DEFINE()`` publishes a Zephyr sensor device and
``BMC_SENSOR_DEFINE()`` a sensor with a custom read callback. Registered
sensors appear in the Redfish sensor collection and under ``bmc sensor`` in the
shell without further code.

**Redfish.** :c:func:`bmc_redfish_identity_register` supplies the product
strings, ``BMC_REDFISH_OEM_DEFINE()`` appends vendor members to a standard
resource, and ``BMC_REDFISH_RESOURCE_DEFINE()`` publishes a resource of its
own.

**HTTP.** ``BMC_HTTP_RESOURCE_DEFINE()`` attaches a resource to both the HTTP
and the HTTPS service, which is how the dashboard in :file:`src/webui.c` is
served.

**Events.** :c:func:`bmc_callback_register` reports boot completion, network
readiness, configuration changes and host power transitions.

**Authentication.** A :c:struct:`bmc_auth_ops` registered with
:c:func:`bmc_auth_ops_register` replaces the built-in password check, for
example with one backed by a secure element.

Configuration
*************

The sample keeps the default build small: HTTPS, JTAG and NTP are disabled and
the configuration is stored with the settings subsystem in NVS.

:kconfig:option:`CONFIG_BMC_SAMPLE_WEB` serves the dashboard. The dashboard
loads xterm.js and Chart.js from :file:`web/vendor/`; those files are not
distributed with Zephyr. Run :file:`web/fetch_vendor.sh` to download them if
the host console and the temperature graph are wanted, see
:file:`web/README.md`. Without them the dashboard still works and the affected
panels explain what is missing.

The dashboard offers two terminals, both of them websockets that the BMC
authenticates with the administrator credentials before the session starts.
The host console panel attaches to the captured host serial line at
``/console/host``, and the BMC shell panel attaches at ``/console/bmc``, served
by :kconfig:option:`CONFIG_SHELL_BACKEND_WEBSOCKET`. The shell there is the
same one as on the serial console, log messages included, so ``bmc config`` and
``bmc sensor`` work from the browser.

:kconfig:option:`CONFIG_BMC_SAMPLE_GENERATE_CERTS` generates a throwaway server
certificate at build time for the HTTPS service, enabled by
:file:`overlay-tls.conf`. It needs ``openssl`` on the build host. Use a real
certificate for anything but a demonstration.

Host serial capture is enabled on ``native_sim`` and ``native_sim/native/64``
through the devicetree overlays in :file:`boards/`, and disabled on
``mps2/an385`` because its CMSDK UART driver does not implement the async UART
API that the console logger requires.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mgmt/bmc
   :host-os: unix
   :board: native_sim
   :goals: build
   :compact:

Run the resulting binary and point a browser or a Redfish client at the BMC:

.. code-block:: console

   $ curl http://192.0.2.1/redfish/v1/
   $ curl -u admin:<password> http://192.0.2.1/redfish/v1/Systems/system

The password is the one in :kconfig:option:`CONFIG_BMC_DEFAULT_ADMIN_PASSWORD`
until it is changed over Redfish or with ``bmc config`` in the shell.

The sample expects a board configuration that provides IPv4 networking. On
boards without a hardware device identifier the subsystem falls back to a
built-in BMC UUID and continues starting normally.
