.. _esp32_wifi_station:

Espressif ESP32 WiFi Station
############################

Overview
********

This sample demonstrates how to use ESP32 to connect to a WiFi network as a station device.
To configure WiFi credentials, edit ``prj.conf``.
Enabling the ``net_shell`` module provides a set of commands to test the connection.
See :ref:`network shell <net_shell>` for more details.

Supported SoCs
**************

The following SoCs are supported by this sample code so far:

* ESP32
* ESP32-S2
* ESP32-C3

Building and Running
********************

Make sure you have the ESP32 connected over USB port.

The sample can be built and flashed as follows:

.. code-block:: console

   west build -b esp32 samples/boards/esp32/wifi_station
   west flash

If using another supported Espressif board, replace the board argument in the above
command with your own board (e.g., :ref:`esp32s2_saola` board).

Sample Output
=============

To check output of this sample, run ``west espressif monitor`` or any other serial console program (i.e. on Linux
minicom, putty, screen, etc).
This example uses ``west espressif monitor``, which automatically detects the serial port at ``/dev/ttyUSB0``:

.. code-block:: console

   $ west espressif monitor

.. code-block:: console

   I (288) boot: Loaded app from partition at offset 0x10000
   I (288) boot: Disabling RNG early entropy source...
   I (611) wifi:wifi driver task: 3ffb2be8, prio:2, stack:3584, core=0
   I (613) wifi:wifi firmware version: 9c89486
   I (613) wifi:wifi certification version: v7.0
   I (614) wifi:config NVS flash: disabled
   I (618) wifi:config nano formatting: disabled
   I (622) wifi:Init data frame dynamic rx buffer num: 32
   I (627) wifi:Init management frame dynamic rx buffer num: 32
   I (632) wifi:Init management short buffer num: 32
   I (636) wifi:Init dynamic tx buffer num: 32
   I (640) wifi:Init static rx buffer size: 1600
   I (645) wifi:Init static rx buffer num: 10
   I (648) wifi:Init dynamic rx buffer num: 32
   phy_version: 4350, 18c5e94, Jul 27 2020, 21:04:07, 0, 2
   I (783) wifi:mode : softAP (24:6f:28:80:32:e9)
   I (784) wifi:Total power save buffer number: 16
   I (784) wifi:Init max length of beacon: 752/752
   I (788) wifi:Init max length of beacon: 752/752
   *** Booting Zephyr OS build zephyr-v2.4.0-49-g4da59e1678f7  ***
   I (798) wifi:mode : sta (24:6f:28:80:32:e8)
   I (1046) wifi:new:<4,1>, old:<1,1>, ap:<255,255>, sta:<4,1>, prof:1
   I (1694) wifi:state: init -> auth (b0)
   I (1711) wifi:state: auth -> assoc (0)
   I (1717) wifi:state: assoc -> run (10)
   I (1745) wifi:connected with myssid, aid = 4, channel 4, 40U, bssid = d8:07:b6:dd:47:7a
   I (1745) wifi:security: WPA2-PSK, phy: bgn, rssi: -57
   I (1747) wifi:pm start, type: 1

   esp_event: WIFI STA start
   esp_event: WIFI STA connected
   I (1813) wifi:AP's beacon interval = 102400 us, DTIM period = 1
   net_dhcpv4: Received: 192.168.68.102
   esp32_wifi_sta: Your address: 192.168.68.102
   esp32_wifi_sta: Lease time: 7200 seconds
   esp32_wifi_sta: Subnet: 255.255.255.0
   esp32_wifi_sta: Router: 192.168.68.1

Sample console interaction
==========================

If the :kconfig:option:`CONFIG_NET_SHELL` option is set, network shell functions
can be used to check internet connection.

.. code-block:: console

   shell> net ping 8.8.8.8
   PING 8.8.8.8
   28 bytes from 8.8.8.8 to 192.168.68.102: icmp_seq=0 ttl=118 time=19 ms
   28 bytes from 8.8.8.8 to 192.168.68.102: icmp_seq=1 ttl=118 time=16 ms
   28 bytes from 8.8.8.8 to 192.168.68.102: icmp_seq=2 ttl=118 time=21 ms
