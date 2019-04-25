.. _wifi_repeater:

Wi-Fi: Repeater
####################

Overview
********

This sample application demonstrates the usage of STA & AP APIs of the Wi-Fi manager.
It demonstrates a simple Wi-Fi repeater, including scanning, connecting an AP
and starting your own AP.

Note that the packet forwarding is accomplished by the Wi-Fi firmware.

Requirements
************

* A board with Wi-Fi and flash support
* An access point with IEEE 802.11 a/b/g/n/ac support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/wifi/repeater` in
the Zephyr tree.

This sample can be built for the boards which support Wi-Fi manager,
in this example we will build it for the ``96b_ivy5661`` board:

.. zephyr-app-commands::
   :zephyr-app: samples/wifi/connect
   :board: 96b_ivy5661
   :goals: build flash
   :compact:

To run this sample, both STA and AP must be configured in advance.

Step 1: Config STA
==================
To specified an open AP, please follow the command below.

.. code-block:: console

    wifimgr set_config sta -n "MERCURY_3BC4" -p "" -a 10000

To specified an WPA/WPA2-PSK secured AP, please follow the command below.

.. code-block:: console

    wifimgr set_config sta -n "MERCURY_3BC4" -p "12345678" -a 10000

.. note::
   For more details, please refer to help of wifimgr.

Step 2: Config AP
=================
To configure the AP as open mode, please follow the command below.

.. code-block:: console

    wifimgr set_config ap -n "UNISOC" -p "" -a 10000

To configure the AP as WPA/WPA2-PSK mode , please follow the command below.

.. code-block:: console

    wifimgr set_config ap -n "UNISOC" -p "12345678" -a 10000

.. note::
   It is suggested to use the same security type as the target AP.

Now restart the board, it will become a Wi-Fi repeater
and begin forwarding packets automatically.

Sample Output
=============

.. code-block:: console

	interface WIFI_STA(40:45:da:00:01:7d) initialized!
	interface WIFI_AP(40:45:da:00:81:7d) initialized!
	WiFi manager started
	starting Wi-Fi Repeater...
	open STA!
	(STA <UNAVAIL>) -> (STA <READY>)
	trgger scan!
	(STA <READY>) -> (STA <SCANNING>)
	SSID:           MERCURY_3BC4
	scan done!
	(STA <SCANNING>) -> (STA <READY>)
	Connecting to MERCURY_3BC4
	(STA <READY>) -> (STA <CONNECTING>)
	start DHCP client
	connect successfully!
	(STA <CONNECTING>) -> (STA <CONNECTED>)
	open AP!
	(AP <UNAVAILABLE>) -> (AP <READY>)
	start AP!
	(AP <READY>) -> (AP <STARTED>)
	done!
	IP address: 192.168.1.120
	Lease time: 7200s
	Subnet: 255.255.255.0
	Router: 192.168.1.1
	[00:00:05.100,000] <inf> net_dhcpv4: Received: 192.168.1.120
