.. zephyr:code-sample:: bluetooth_isochronous_connected_benchmark
   :name: Isochronous Connected Channels Benchmark
   :relevant-api: bt_iso bluetooth

   Measure packet loss and sync loss in connected ISO channels.

The ISO Connected Channels Benchmark sample measures and reports packet loss
and sync loss in connected ISO channels.

Overview
********

The sample transmits data between the *central* and the *peripheral*
and measures the quality of the ISO channels.

The application can be used as both a central and a peripheral, and the mode
can easily be changed.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support
* A Bluetooth Controller and board that supports setting
  CONFIG_BT_CTLR_CENTRAL_ISO=y
* A remote board running the same sample as the reversed role that supports
  setting CONFIG_BT_CTLR_PERIPHERAL_ISO=y

Building and running
********************

This sample can be found under
:zephyr_file:`samples/bluetooth/iso_connected_benchmark` in the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.


Testing
=======

After programming the sample to both boards, test it by performing the following
steps:

1. Connect to both boards with a terminal emulator (for example, PuTTY or
   minicom).
#. Reset both boards.
#. In one of the terminal emulators, type "c" to start the application on the
   connected board in the central role.
#. In the other terminal emulator, type "p" to start the application in the
   peripheral role.
#. Optionally modify the central ISO settings using the console.
#. Observe that the central and the peripheral connects.
#. Observe the receive statistics on the devices (the connected ISO channels may
   by uni- or bidirectional).

Sample output
==============
The peripheral will output overall data (since boot),
current connection (since the CIG was connected) and latest 1000 received
packets::

  *** Booting Zephyr OS build zephyr-v2.5.0-4098-gdcaaee6db2f5  ***
   [00:00:00.392,333] <inf> iso_connected: Starting Bluetooth Throughput example
   [00:00:00.405,395] <inf> iso_connected: Bluetooth initialized
   Choose device role - type c (central role) or p (peripheral role), or q to quit: p
   Peripheral role
   [00:00:10.281,555] <inf> iso_connected: Registering ISO server
   [00:00:10.281,555] <inf> iso_connected: Starting advertising
   [00:00:10.282,684] <inf> iso_connected: Waiting for ACL connection
   [00:00:16.711,517] <inf> iso_connected: Connected: FA:4C:4B:DB:D3:89 (public)
   [00:00:16.711,669] <inf> iso_connected: Waiting for ISO connection
   [00:00:16.802,856] <inf> iso_connected: Incoming ISO request
   [00:00:16.802,856] <inf> iso_connected: Returning instance 0
   [00:00:17.016,845] <inf> iso_connected: ISO Channel 0x20002934 connected
   [00:00:17.769,439] <inf> iso_connected: Sending value 100
   [00:00:17.774,902] <inf> iso_connected: Overall     : Received 100/100 (100.00%) - Total packets lost 0
   [00:00:17.774,932] <inf> iso_connected: Current Conn: Received 100/100 (100.00%) - Total packets lost 0
   [00:00:17.774,963] <inf> iso_connected: Latest 1000 : Received 100/100 (100.00%) - Total packets lost 0
   [00:00:17.774,963] <inf> iso_connected:
   [00:00:18.529,510] <inf> iso_connected: Sending value 200
   [00:00:18.532,409] <inf> iso_connected: Overall     : Received 200/200 (100.00%) - Total packets lost 0
   [00:00:18.532,470] <inf> iso_connected: Current Conn: Received 200/200 (100.00%) - Total packets lost 0
   [00:00:18.532,501] <inf> iso_connected: Latest 1000 : Received 200/200 (100.00%) - Total packets lost 0


The central will ask if any changes to the current settings are wanted.
If y/Y is chosen, then it will create a prompt to changes the settings,
otherwise continue with the current settings. The central will then start
scanning for and connecting to a peer device running the sample as peripheral
role. Once connected, the central will output overall data (since boot),
current connection (since the CIG was connected) and latest 1000 received
packets::

   *** Booting Zephyr OS build zephyr-v2.5.0-4098-gdcaaee6db2f5  ***
   [00:00:00.459,869] <inf> iso_connected: Starting Bluetooth Throughput example
   [00:00:00.472,961] <inf> iso_connected: Bluetooth initialized
   Choose device role - type c (central role) or p (peripheral role), or q to quit: c
   Central role
   Change ISO settings (y/N)?
   [00:00:03.277,893] <inf> iso_connected: Scan started
   [00:00:03.277,893] <inf> iso_connected: Waiting for advertiser
   [00:00:03.899,963] <inf> iso_connected: Found peripheral with address F4:5A:12:BF:4F:2C (public) (RSSI -24)
   [00:00:03.900,024] <inf> iso_connected: Stopping scan
   [00:00:03.908,020] <inf> iso_connected: Scan stopped
   [00:00:03.908,020] <inf> iso_connected: Connecting
   [00:00:04.007,232] <inf> iso_connected: Connected: F4:5A:12:BF:4F:2C (public)
   [00:00:04.007,354] <inf> iso_connected: Binding ISO
   [00:00:04.007,812] <inf> iso_connected: Connecting ISO channels
   [00:00:04.312,744] <inf> iso_connected: ISO Channel 0x20002934 connected
   [00:00:05.065,368] <inf> iso_connected: Sending value 100
   [00:00:05.072,052] <inf> iso_connected: Overall     : Received 100/100 (100.00%) - Total packets lost 0
   [00:00:05.072,113] <inf> iso_connected: Current Conn: Received 100/100 (100.00%) - Total packets lost 0
   [00:00:05.072,143] <inf> iso_connected: Latest 1000 : Received 100/100 (100.00%) - Total packets lost 0
   [00:00:05.072,143] <inf> iso_connected:
   [00:00:05.825,439] <inf> iso_connected: Sending value 200
   [00:00:05.829,589] <inf> iso_connected: Overall     : Received 200/200 (100.00%) - Total packets lost 0
   [00:00:05.829,620] <inf> iso_connected: Current Conn: Received 200/200 (100.00%) - Total packets lost 0
   [00:00:05.829,650] <inf> iso_connected: Latest 1000 : Received 200/200 (100.00%) - Total packets lost 0
