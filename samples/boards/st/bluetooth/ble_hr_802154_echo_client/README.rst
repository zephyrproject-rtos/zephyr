.. zephyr:code-sample:: stm32_ble_hr_802154_echo_client
   :name: BLE HR - 802154 Echo client (advanced)

   Implement a Bluetooth |reg| Energy Heart Rate - IEEE 802154 client.
   The BLE Heart Rate (HR) GATT Service generates dummy heart-rate
   values and the 802154 echo client sends IP packets, waits for data
   to be sent back, and verifies it.

Overview
********

The sample application implements a concurrent mode BLE - IEEE 802.15.4.
Both wireless protocols Bluetooth |reg| Energy and IEEE 802.15.4 coexist :
the application supports simultaneously one Bluetooth |reg| Energy connection
and one IEEE 802.15.4 connection.
The 802.15.4 part implements UDP client that will send IPv6 packets,
wait for the data to be sent back, and then verify it matches
the data that was sent.
The BLE part exposes the HR (Heart Rate) GATT Service. Once a device
connects it will generate dummy heart-rate values.

The source code for this sample application can be found at:
:zephyr_file:`samples/boards/st/ble_802154/ble_hr_802154_echo_client`.

The IEEE 802.15.4 config of the sample enables support for native
IEEE 802.15.4 connectivity.
Note that by default IEEE 802.15.4 L2 uses unacknowledged communication.
To improve connection reliability, acknowledgments can be enabled with
shell command: ``ieee802154 ack set``.

Environment Setup
*****************

This sample has been tested on the STMicroelectonics NUCLEO-WBA65RI board
(nucleo_wba65ri).
This board interacts simultaneously with another board embedding an
echo-server sample with the 802.15.4 overlay (nucleo_wba65ri) and a
smartphone supporting BLE Heart Rate.

Building and Running
********************

Build sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/ble_802154/ble_hr_802154_echo_client
   :board: nucleo_wba65ri
   :goals: build
   :compact:

The LED 1 toggles while application is BLE advertising. Once a remote device
connects, the LED 1 turns ON and application will generate dummy heart-rate values.
Once remote device disconnects, the application restarts BLE advertising and LED 1
toggles.

Simultaneously, IEEE 802.15.4 feature is enabled. To validate behavior, use another
board that will be used as a remote server.
This can be done using echo-server sample application with :file:`overlay-802154.conf`.

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :board: nucleo_wba65ri
   :gen-args: -DEXTRA_CONF_FILE=overlay-802154.conf
   :goals: build flash
   :compact:

Once both devices are connected, local device will send IPv6 packets, wait for
the data to be sent back from the remote echo-server device, and then verify it
matches the data that was sent.
