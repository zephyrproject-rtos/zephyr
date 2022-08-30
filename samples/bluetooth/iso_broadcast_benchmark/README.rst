.. _iso_broadcast_benchmark:

Bluetooth: Throughput
#####################

The ISO Broadcast Benchmark sample measures and report packet loss and sync loss
of an ISO broadcaster against one or more ISO broadcast receivers.

Overview
********

The sample transmits data from the *broadcaster* to the
*receiver* (multiple receivers may be used with no effect on the test)
and measures the quality of the sync.

The application is used as both a broadcaster and a receiver, and the mode
can easily be changed.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support
* A Bluetooth Controller and board that supports setting
  CONFIG_BT_CTLR_ADV_ISO=y
* A remote board running the same sample as the reversed role that supports
  setting CONFIG_BT_CTLR_SYNC_ISO

Building and running
********************

This sample can be found under
:zephyr_file:`samples/bluetooth/iso_broadcast_benchmark` in the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.


Testing
=======

After programming the sample to both boards, test it by performing the following
steps:

1. Connect to both boards with a terminal emulator (for example, PuTTY or
   minicom).
#. Reset both boards.
#. In one of the terminal emulators, type "r" to start the application on the
   connected board in the receiver role.
#. In the other terminal emulator, type "b" to start the application in the
   broadcaster role.
#. Optionally modify the broadcasting settings.
#. Observe that the receiver synchronizes to the broadcaster.
#. Observe the receive statistics on the receiver(s).

Sample output
==============
The receiver will output statistics for overall (since boot), current sync
(since the BIG was synced) and latest 1000 received packets::

   *** Booting Zephyr OS build zephyr-v2.5.0-3768-g42f09317bfbe  ***
   [00:00:00.447,845] <inf> iso_broadcast_main: Starting Bluetooth Throughput
   Choose device role - type r (receiver role) or b (broadcaster role), or q to quit: r
   Receiver role
   [00:00:05.784,698] <inf> iso_broadcast_receiver: Scan started
   [00:00:05.784,698] <inf> iso_broadcast_receiver: Waiting for periodic advertiser
   [00:00:05.793,304] <inf> iso_broadcast_receiver: Found broadcaster with address 28:3B:AD:F5:EE:0C (random) (RSSI -33)
   [00:00:05.793,334] <inf> iso_broadcast_receiver: Periodic advertiser found
   [00:00:05.793,701] <inf> iso_broadcast_receiver: Scan stopped
   [00:00:05.793,701] <inf> iso_broadcast_receiver: Creating Periodic Advertising Sync
   [00:00:05.796,081] <inf> iso_broadcast_receiver: Waiting for periodic sync
   [00:00:06.453,460] <err> bt_scan: Unknown handle 0x0000 for periodic advertising report
   [00:00:06.453,979] <inf> iso_broadcast_receiver: Periodic advertisement synced
   [00:00:06.454,010] <inf> iso_broadcast_receiver: Periodic sync established
   [00:00:06.454,040] <inf> iso_broadcast_receiver: Waiting for BIG info
   [00:00:08.853,485] <inf> iso_broadcast_receiver: BIGinfo received: num_bis 2, nse 1, interval 7 ms, bn 1, pto 0, irc 1, max_pdu 251, sdu_interval 7500 us, max_sdu 251, phy LE 2M, without framing, not encrypted
   [00:00:08.853,515] <inf> iso_broadcast_receiver: Syncing to BIG
   [00:00:08.853,973] <inf> iso_broadcast_receiver: Waiting for BIG sync
   [00:00:11.254,211] <inf> iso_broadcast_receiver: ISO Channel 0x2000124c connected
   [00:00:11.254,333] <inf> iso_broadcast_receiver: BIG sync established
   [00:00:11.254,486] <inf> iso_broadcast_receiver: ISO Channel 0x20001260 connected
   [00:00:11.639,343] <inf> iso_broadcast_receiver: Overall     : Received 99/100 (99.00%) - Total packets lost 1
   [00:00:11.639,343] <inf> iso_broadcast_receiver: Current Sync: Received 99/100 (99.00%) - Total packets lost 1
   [00:00:11.639,373] <inf> iso_broadcast_receiver: Latest 1000 : Received 99/100 (99.00%) - Total packets lost 1
   [00:00:12.029,388] <inf> iso_broadcast_receiver: Overall     : Received 199/200 (99.50%) - Total packets lost 1
   [00:00:12.029,388] <inf> iso_broadcast_receiver: Current Sync: Received 199/200 (99.50%) - Total packets lost 1
   [00:00:12.029,388] <inf> iso_broadcast_receiver: Latest 1000 : Received 199/200 (99.50%) - Total packets lost 1


The broadcaster will ask if any changes to the current settings are wanted.
If y/Y is chosen, then it will create a prompt to enter changes to the settings,
otherwise continue with the current settings. The broadcaster will then start
the BIG and output the current counter (since the BIG was created)::

   *** Booting Zephyr OS build zephyr-v2.5.0-3768-g06d4327cc601  ***
   [00:00:00.447,845] <inf> iso_broadcast_main: Starting Bluetooth Throughput
   Choose device role - type r (receiver role) or b (broadcaster role), or q to quit: b
   Broadcaster role
   Change settings (y/N)? (Current settings: rtn=2, interval=7500, latency=10, phy=2, sdu=251, packing=0, framing=0, bis_count=2)
   [00:00:08.802,185] <inf> iso_broadcast_broadcaster: Creating Extended Advertising set
   [00:00:08.804,260] <inf> iso_broadcast_broadcaster: Setting Extended Advertising parameters
   [00:00:08.804,504] <inf> iso_broadcast_broadcaster: Starting Periodic Advertising
   [00:00:08.804,870] <inf> iso_broadcast_broadcaster: Starting Extended Advertising set
   [00:00:08.807,159] <inf> iso_broadcast_broadcaster: Creating BIG
   [00:00:08.807,617] <inf> iso_broadcast_broadcaster: Waiting for BIG complete
   [00:00:08.813,049] <inf> iso_broadcast_broadcaster: ISO Channel 0x20001218 connected
   [00:00:08.813,171] <inf> iso_broadcast_broadcaster: BIG created
   [00:00:08.813,507] <inf> iso_broadcast_broadcaster: ISO Channel 0x2000122c connected
   [00:00:09.196,472] <inf> iso_broadcast_broadcaster: Sending value 100
   [00:00:09.587,036] <inf> iso_broadcast_broadcaster: Sending value 200
   [00:00:09.977,722] <inf> iso_broadcast_broadcaster: Sending value 300
   [00:00:10.368,347] <inf> iso_broadcast_broadcaster: Sending value 400
   [00:00:10.758,972] <inf> iso_broadcast_broadcaster: Sending value 500
   [00:00:11.149,597] <inf> iso_broadcast_broadcaster: Sending value 600
   [00:00:11.540,222] <inf> iso_broadcast_broadcaster: Sending value 700
