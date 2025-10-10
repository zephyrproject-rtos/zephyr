.. zephyr:code-sample:: stm32_ble_hr_802154_echo_client
   :name: Ble HR - 802154 Echo client (advanced)

   Implement a Bluetooth Heart Rate - 802154 client.
   The Bluetooth Heart Rate (HR) GATT Service generates dummy heart-rate
   values and the 802154 echo client sends IP packets, waits for data
   to be sent back, and verifies it.

Overview
********

The sample application for Zephyr implements a concurrent mode
BLE - IEEE802.15.4.
The 802.15.4 part implements UDP/TCP client that will send IPv4
or IPv6 packets, wait for the data to be sent back, and then verify
it matches the data that was sent.
The BLE part exposes the HR (Heart Rate) GATT Service. Once a device
connects it will generate dummy heart-rate values.

The source code for this sample application can be found at:
:zephyr_file:`samples/boards/st/ble_802154/ble_hr_802154_echo_client`.

Requirements
************

- :ref:`networking_with_host`
* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************


There are configuration files for different boards and setups in the
samplet directory:

- :file:`prj.conf`
  Generic config file, normally you should use this.

- :file:`overlay-802154.conf`
  This overlay config enables support for native IEEE 802.15.4 connectivity.
  Note, that by default IEEE 802.15.4 L2 uses unacknowledged communication. To
  improve connection reliability, acknowledgments can be enabled with shell
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
   :gen-args: -DEXTRA_CONF_FILE=overlay-802154.conf
   :goals: build
   :compact:

In a terminal window you can check if communication is happen:

.. code-block:: console

    $ minicom -D /dev/ttyACM1
