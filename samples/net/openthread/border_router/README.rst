.. zephyr:code-sample:: openthread-border-router
   :name: OpenThread BorderRouter
   :relevant-api:

   Test Wi-Fi and IEEE 802.15.4 using the OpenThread Border Router.

Overview
********

This sample allows testing the Thread protocol and the Thread Border Router functionality for various
boards using the OpenThread and Wi-Fi shell.

Building and Running
********************

Verify that the board and chip you are targeting provide IEEE 802.15.4 and Wi-Fi support.

There are configuration files for different boards and setups in the shell directory:

- :file:`prj.conf`
  Generic config file.

- :file:`ot-rcp-host-wifi-nxp.conf`
  This extra config file enables support of OpenThread RCP host running on NXP chips over IMU
  interface. It also has Wi-Fi support enabled.

- :file:`webserver.conf`
  This extra config file enables the HTTP web server (REST APIs).

Build OpenThread Border Router application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/border_router
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for NXP's RW612 FRDM (RCP host).

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/border_router
   :board: frdm_rw612
   :conf: "prj.conf ot-rcp-host-wifi-nxp.conf"
   :goals: build
   :compact:

Example building with Web Server enabled for NXP's RW612 FRDM (RCP host).

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/border_router
   :board: frdm_rw612
   :conf: "prj.conf ot-rcp-host-wifi-nxp.conf webserver.conf"
   :goals: build
   :compact:

MIMXRT1060-EVK Rev-C with IW612/IW610 M.2 modules
==================================================

The MIMXRT1060-EVK Rev-C runs the OpenThread host with an IW612 (Murata 2EL) or
IW610 (Murata 2LL) M.2 module providing the 802.15.4 RCP over SPI. The build
uses the ``nxp_m2_2ll_wifi_bt`` and ``nxp_m2_2el_2ll_ieee802154`` shields and the
fragments in :file:`boards/mimxrt1060_evk_mimxrt1062_qspi_C/`, which select the
backbone (``wifi.conf`` or ``eth.conf``) and the module (``iw612.conf`` or
``iw610.conf``).

Example building for an IW612 module with a wired Ethernet backbone:

.. code-block:: console

   west build -p -b mimxrt1060_evk@C/mimxrt1062/qspi \
       --shield "nxp_m2_2ll_wifi_bt nxp_m2_2el_2ll_ieee802154" \
       samples/net/openthread/border_router \
       -- -DEXTRA_CONF_FILE=" \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/mimxrt1060_evk_mimxrt1062_qspi_C.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/eth.conf \
       boards/mimxrt1060_evk_mimxrt1062_qspi_C/iw612.conf"

All four module and backbone combinations are described in
:ref:`nxp_m2_ieee802154`.

Web Server
**********

When built with ``webserver.conf``, the application includes an HTTP web server providing:

- **Web Interface**: Accessible at ``http://<device-ip>:8080`` or ``http://zephyr-otbr:8080`` (via mDNS)
- **REST API**: JSON-based endpoints for Thread network management
