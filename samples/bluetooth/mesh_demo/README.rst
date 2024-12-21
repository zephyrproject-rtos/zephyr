.. zephyr:code-sample:: ble_mesh_demo
   :name: Mesh Demo
   :relevant-api: bt_mesh bluetooth

   Implement a Bluetooth Mesh demo application.

Overview
********

This sample is a Bluetooth Mesh application intended for demonstration
purposes only. The application provisions and configures itself (i.e. no
external provisioner needed) with hard-coded network and application key
values. The local unicast address can be set using a NODE_ADDR build
variable (e.g. NODE_ADDR=0x0001 for unicast address 0x0001), or by
manually editing the value in the ``board.h`` file.

Because of the hard-coded values, the application is not suitable for
production use, but is quite convenient for quick demonstrations of mesh
functionality.

The application has some features especially designed for the BBC
micro:bit boards, such as the ability to send messages using the board's
buttons as well as showing information of received messages on the
board's 5x5 LED display. It's generally recommended to use unicast
addresses in the range of 0x0001-0x0009 for the micro:bit since these
map nicely to displayed addresses and the list of destination addresses
which can be cycled with a button press.

A special address, 0x000f, will make the application become a heart-beat
publisher and enable the other nodes to show information of the received
heartbeat messages.

Requirements
************

* A board with Bluetooth LE support, or
* QEMU with BlueZ running on the host

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/mesh_demo` in
the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details on how
to run the sample inside QEMU.

For other boards, build and flash the application as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/mesh_demo
   :board: <board>
   :goals: flash
   :compact:

Refer to your :ref:`board's documentation <boards>` for alternative
flash instructions if your board doesn't support the ``flash`` target.

To run the application on an :ref:`nrf5340dk_nrf5340`, a Bluetooth controller application
must also run on the network core. The :zephyr:code-sample:`bluetooth_hci_ipc` sample
application may be used. Build this sample with configuration
:zephyr_file:`samples/bluetooth/hci_ipc/nrf5340_cpunet_bt_mesh-bt_ll_sw_split.conf`
to enable mesh support.
