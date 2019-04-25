.. _wifi_sta_rtt:

Wi-Fi: RTT range
####################

Overview
********

This sample application shows how to use the RTT range APIs of Wi-Fi manager .
It demonstrates several main features: scanning APs with RTT support
and requesting RTT range.

Requirements
************

* A board with Wi-Fi and flash support
* One or more access points with IEEE 802.11 a/b/g/n/ac/mc support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/wifi/rtt` in
the Zephyr tree.

This sample can be built for the boards which support Wi-Fi manager,
in this example we will build it for the ``96b_ivy5661`` board:

.. zephyr-app-commands::
   :zephyr-app: samples/wifi/connect
   :board: 96b_ivy5661
   :goals: build flash
   :compact:

To run this sample, the STA must be configured in advance.

.. code-block:: console

    wifimgr set_config sta -a 1000

.. note::
   For more details, please refer to help of wifimgr.

Now restart the board, it will request RTT range repeatedly.

Sample Output
=============

.. code-block:: console

	interface WIFI_STA(40:45:da:00:02:1e) initialized!
	WiFi manager started
	start RTT range...
	open STA!
	(STA <UNAVAIL>) -> (STA <READY>)
	trgger scan!
	(STA <READY>) -> (STA <SCANNING>)
		UNISOC          40:45:da:00:81:7d       161
	scan done!
	(STA <SCANNING>) -> (STA <READY>)
	request RTT range!
	(STA <READY>) -> (STA <RTTING>)
		40:45:da:00:81:7d       28
	RTT done!
	(STA <RTTING>) -> (STA <READY>)
	trgger scan!
	(STA <READY>) -> (STA <SCANNING>)
		UNISOC          40:45:da:00:81:7d       161
	scan done!
	(STA <SCANNING>) -> (STA <READY>)
	request RTT range!
	(STA <READY>) -> (STA <RTTING>)
		40:45:da:00:81:7d       20
	RTT done!
	(STA <RTTING>) -> (STA <READY>)
	trgger scan!
	(STA <READY>) -> (STA <SCANNING>)
		UNISOC          40:45:da:00:81:7d       161
	scan done!
	(STA <SCANNING>) -> (STA <READY>)
	request RTT range!
	(STA <READY>) -> (STA <RTTING>)
		40:45:da:00:81:7d       25
	RTT done!
	(STA <RTTING>) -> (STA <READY>)
