.. _wifi_sta_connect:

Wi-Fi: Auto Connect
####################

Overview
********

This sample application shows how to use the Wi-Fi manager APIs to connect an AP.
It demonstrates several main features: getting STA config, opening STA,
scanning, connecting an AP.

Requirements
************

* A board with Wi-Fi and flash support
* An access point with IEEE 802.11 a/b/g/n/ac support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/wifi/connect` in
the Zephyr tree.

This sample can be built for the boards which support Wi-Fi manager,
in this example we will build it for the ``96b_ivy5661`` board:

.. zephyr-app-commands::
   :zephyr-app: samples/wifi/connect
   :board: 96b_ivy5661
   :goals: build flash
   :compact:

To run this sample, the STA must be configured in advance.
To specified an open AP, please follow the command below.

.. code-block:: console

    wifimgr set_config sta -n "MERCURY_3BC4" -p "" -a 10000

To specified an WPA/WPA2-PSK secured AP, please follow the command below.

.. code-block:: console

    wifimgr set_config sta -n "MERCURY_3BC4" -p "12345678" -a 10000

.. note::
   For more details, please refer to help of wifimgr.

Now restart the board, it will connect to the target AP automatically.

Sample Output
=============

.. code-block:: console

	interface WIFI_STA(40:45:da:00:01:7d) initialized!
	WiFi manager started
	Auto connecting...
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
	IP address: 192.168.1.120
	Lease time: 7200s
	Subnet: 255.255.255.0
	Router: 192.168.1.1
	[00:00:09.270,000] <inf> net_dhcpv4: Received: 192.168.1.120
