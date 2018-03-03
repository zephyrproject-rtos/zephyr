.. _simplelink_shell:

SimpleLink WLAN Shell for CC3220SF WiFi
########################################

Overview
********

This sample enables testing of the WiFi offload management operations
of the SimpleLink WiFi driver, and shows how to request and get
notifications of WLAN events using the Zephyr network management
framework, :ref:`network_management_api`.

Building and Running
********************

This sample implements a simple shell, prompting input from the console.
It can be built and executed on the CC3220SF_LAUNCHXL as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/cc3220sf_launchxl/simplelink_shell
   :board: cc3220sf_launchxl
   :goals: build
   :compact:

Sample Console Interaction
==========================

.. code-block:: console

   cc3220sf wifi> scan
   Executing: scan
   Ind | SSID                             | Ch   | RSSI | Sec
   0   | NETGEAR88_2GEXT                  | 7    | -82  | PSK
   1   | NETGEAR26                        | 10   | -73  | PSK
   2   | GP-Galaxy                        | 6    | -33  | PSK
   3   | JDB-Net-DL                       | 6    | -70  | PSK
   ----------
   cc3220sf wifi> connect GP-Galaxy mypassword
   Executing: connect
   Connect succeeded
   IPv4 address: 192.168.45.145
   IPv4 gateway: 192.168.45.1
   cc3220sf wifi> disconnect
   Executing: disconnect
   Disconnect succeeded
