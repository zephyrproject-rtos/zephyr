.. zephyr:code-sample:: wifi-ap-sta-mode
   :name: Wi-Fi AP-STA mode
   :relevant-api: wifi_mgmt dhcpv4_server

   Configure a Wi-Fi board to operate as both an Access Point (AP) and a Station (STA).

Overview
********

The Wi-Fi AP-STA mode of a Wi-Fi board allows it to function as both
an Access Point (AP) and a Station (STA) simultaneously.
This sample demonstrates how to configure and utilize AP-STA mode.

Configuration and usage of following interfaces is shown in sample.

1. ``AP mode``: AP mode is configured and enabled. DHCPv4 server is also
   configured to assign IP addresses to the joining station.
2. ``STA mode``: Provide the SSID and PSK of you router

In this demo, AP-STA mode is enabled using :kconfig:option:`CONFIG_ESP32_WIFI_AP_STA_MODE`.
An additional Wi-Fi node is added in the ``.overlay`` file. The ``net_if``.
In the sample code, initially, the AP mode is enabled, followed by enabling the STA mode.
The driver checks if AP mode was previously enabled. If so, it transitions
the board into AP-STA mode to support both modes and attempts to connect to the
AP specified by the provided SSID and PSK.

Requirements
************

This example should be able to run on any commonly available
:zephyr:board:`esp32_devkitc_wroom` development board without any extra hardware.

To enable or disable ``AP-STA`` mode, modify the :kconfig:option:`CONFIG_ESP32_WIFI_AP_STA_MODE`
parameter in the ``boards/esp32_devkitc_wroom_procpu.conf`` file of the demo. Moreover, an
extra Wi-Fi node is included in ``boards/esp32_devkitc_wroom_procpu.overlay``.

By default, AP-STA mode is disabled.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi/apsta_mode
   :board: esp32_devkitc_wroom/esp32/procpu
   :goals: build flash
   :compact:

Sample Output
=================

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0-rc3-104-gd1e5c5b3f9b7 ***
   [00:00:05.171,000] <inf> MAIN: Turning on AP Mode
   [00:00:05.172,000] <dbg> net_dhcpv4_server: net_dhcpv4_server_start: Started DHCPv4 server, address pool:
   [00:00:05.172,000] <dbg> net_dhcpv4_server: net_dhcpv4_server_start:     0: 192.168.4.11
   [00:00:05.172,000] <dbg> net_dhcpv4_server: net_dhcpv4_server_start:     1: 192.168.4.12
   [00:00:05.172,000] <dbg> net_dhcpv4_server: net_dhcpv4_server_start:     2: 192.168.4.13
   [00:00:05.172,000] <dbg> net_dhcpv4_server: net_dhcpv4_server_start:     3: 192.168.4.14
   [00:00:05.172,000] <inf> MAIN: DHCPv4 server started...

   [00:00:05.350,000] <inf> MAIN: AP Mode is enabled. Waiting for sta to connect ESP32-AP
   [00:00:05.350,000] <inf> MAIN: Connecting to SSID: ZIN-Dummy

   [00:00:09.498,000] <inf> net_dhcpv4: Received: 192.168.43.44
   [00:00:09.499,000] <inf> MAIN: Connected to ZIN-Dummy
   [00:00:32.739,000] <inf> MAIN: station: 7C:50:79:17:89:19 joined
   [00:00:32.832,000] <dbg> net_dhcpv4_server: dhcpv4_handle_discover: DHCPv4 processing Discover - reserved 192.168.4.11
   [00:00:33.839,000] <dbg> net_dhcpv4_server: dhcpv4_handle_request: DHCPv4 processing Request - allocated 192.168.4.11
