.. _infineon_wifi_station:

Infineon WiFi Station
############################

Overview
********

This sample demonstrates how to use Infineon cy8cproto_062_4343w to connect to a WiFi network as a station device.
To configure WiFi credentials, edit ``prj.conf``.
Enabling the ``net_shell`` module provides a set of commands to test the connection.
See :ref:`network shell <net_shell>` for more details.

Supported SoCs
**************

The following SoCs are supported by this sample code so far:

* infineon_cat1

Building and Running
********************

Make sure you have the cy8cproto_062_4343w connected over USB port.

The sample can be built and flashed as follows:

.. code-block:: console

   west build -b cy8cproto_062_4343w samples/boards/infineon/wifi_station
   west flash


Sample Output
=============

To check output of this sample, Open serial console program and select the proper COM port(i.e. on Linux
minicom, putty, screen, etc).

.. code-block:: console

WLAN MAC Address : 00:9D:6B:89:56:60
WLAN Firmware    : wl0: Jul 18 2021 19:15:39 version 7.45.98.120 (56df937 CY) FWID 01-69db62cf
WLAN CLM         : API: 12.2 Data: 9.10.39 Compiler: 1.29.4 ClmImport: 1.36.3 Creation: 2021-07-18 19:03:20
WHD VERSION      : v2.2.0-dirty : v2.2.0 : GCC 9.3 : 2021-12-14 13:57:54 +0000
Connected to AP [WIFI_SSID] BSSID : 3C:37:86:FC:07:3E , RSSI -36
*** Booting Zephyr OS build 971d6a1ae3fd  ***


Sample console interaction
==========================

If the :kconfig:`CONFIG_NET_SHELL` option is set, network shell functions
can be used to check internet connection.

.. code-block:: console

   shell> net ping 8.8.8.8
   PING 8.8.8.8
   28 bytes from 8.8.8.8 to 192.168.68.102: icmp_seq=0 ttl=118 time=19 ms
   28 bytes from 8.8.8.8 to 192.168.68.102: icmp_seq=1 ttl=118 time=16 ms
   28 bytes from 8.8.8.8 to 192.168.68.102: icmp_seq=2 ttl=118 time=21 ms
