.. zephyr:code-sample:: stm32_ble_hr_802154_echo_client
   :name: BLE HR - 802154 Echo client (advanced)

   Implement a Bluetooth |reg| Energy Heart Rate - IEEE 802154 client.
   The BLE Heart Rate (HR) GATT Service generates dummy heart-rate
   values and the 802154 echo client sends IP packets, waits for data
   to be sent back, and verifies it.

Overview
********

The sample application for Zephyr implements a concurrent mode
BLE - IEEE 802.15.4.
The 802.15.4 part implements UDP client that will send IPv6 packets,
wait for the data to be sent back, and then verify it matches
the data that was sent.
The BLE part exposes the HR (Heart Rate) GATT Service. Once a device
connects it will generate dummy heart-rate values.

The source code for this sample application can be found at:
:zephyr_file:`samples/boards/st/ble_802154/ble_hr_802154_echo_client`.

Requirements
************

This sample has been tested on the STMicroelectonics NUCLEO-WBA65RI board
(nucleo_wba65ri).

Building and Running
********************

There are configuration files for different boards and setups in the
samples directory:

- :file:`prj.conf`
  This config enables support for native IEEE 802.15.4 connectivity.
  Note that by default IEEE 802.15.4 L2 uses unacknowledged communication.
  To improve connection reliability, acknowledgments can be enabled with shell
  command: ``ieee802154 ack set``.

Build sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/ble_802154/ble_hr_802154_echo_client
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for the IEEE 802.15.4 on nucleo_wba65ri:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/ble_802154/ble_hr_802154_echo_client
   :board: nucleo_wba65ri
   :goals: build
   :compact:

The LED 1 toggles while application is BLE advertising. Once a remote device
connects, the LED 1 turns ON and application will generate dummy heart-rate values.
Once remote device disconnects, the application restarts BLE advertising and LED 1
toggles.

Simultaneously, IEEE 802.15.4 feature is enabled.
In remote device, you can run a echo-server sample application with
:file:`overlay-802154.conf`.
Once both devices are connected, local device will send IPv6 packets,wait for
the data to be sent back from the remote echo-server device, and then verify it
matches the data that was sent.
