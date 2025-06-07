.. zephyr:code-sample:: siwx91x_sample_app
   :name: siwx91x sample app

Overview
********

A sample application that demonstrates the DNS client and ICMP functionality.
This program will get the IP address of the given hostname and the ICMP will
initiate a PING request to the address obtained from the DNS client.

Requirements
************

* Windows PC (Remote PC).
* SiWx91x Wi-Fi Evaluation Kit(SoC).

Configuration Parameters
************************
The below confgurations are available in app_config.h file

#define SSID 				"SiWx91x_AP"		                  // Wi-Fi Network Name
#define PSK	 				"12345678"		                  // Wi-Fi Password
#define SECURITY_TYPE	WIFI_SECURITY_TYPE_PSK			   // Wi-Fi Security Type: WIFI_SECURITY_TYPE_NONE/WIFI_SECURITY_TYPE_WPA_PSK/WIFI_SECURITY_TYPE_PSK
#define CHANNEL_NO		WIFI_CHANNEL_ANY				      // Wi-Fi channel
#define HOSTNAME			"www.zephyrproject.org"          //Hostname to ping

Building and Running
********************

* This sample can be found under :zephyr_file:`zephyr/samples/boards/siwx91x/siwx91x_sample_app` in the Zephyr tree.

* Build:- west build -b siwx917_rb4338a samples/boards/siwx91x/siwx91x_sample_app/ -p

* Flash:- west flash

Test the Application
********************
* Open any serial console to view logs.
* After program gets executed, SiWx91x EVK will be connected to an Access Point with configured **SSID** and **PSK**
* The device will start sending ping requests to the given hostname and the response can be seen in network_event_handler().
