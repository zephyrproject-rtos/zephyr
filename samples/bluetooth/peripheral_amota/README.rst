.. _peripheral_amota:

Bluetooth: Peripheral AMOTA
###########################

Overview
********

A sample to demonstrate the Ambiq BLE AMOTA service. The Ambiq
OTA APP will discover the AMOTA service after connecting with
the board and the user can load the binary which is for updating
and pre-stored in the smartphone from the APP home page. The APP
will show progress bar if the OTA has been started successfully.

Requirements
************

* An apollo4p_blue_kxr_evb or Apollo510b_evb
* An Android or iOS smartphone installed with Ambiq OTA APP

Building and Running
********************

This application can be built and executed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_amota
   :board: apollo4p_blue_kxr_evb

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_amota
   :board: Apollo510b_evb

Sample Output
=============

.. code-block:: console

   BLE AMOTA Application
   bt_controller: BLE Controller Info:
   bt_controller: 	FW Ver:      1.21.2.0
   bt_controller: 	Chip ID0:    0x92492492
   bt_controller: 	Chip ID1:    0x4190d413
   bt_controller: No new image to upgrade
   bt_controller: BLE Controller FW Auth Passed, Continue with FW
   bt_apollox_driver: BT controller initialized
   bt_hci_core: Identity: 22:89:67:45:23:22 (public)
   bt_hci_core: HCI: version 5.1 (0x0a) revision 0x0115, manufacturer 0x09ac
   bt_hci_core: LMP: version 5.1 (0x0a) subver 0x0115
   Bluetooth initialized
   Advertising successfully started
   amota: Updated MTU: TX: 23 RX: 23 bytes
   Connected
   amota: AMOTA notifications enabled
   amota: Packet received correctly, OTA is ongoing...
   amota: OTA process start from beginning
   amota: ============= fw header start ===============
   amota: encrypted = 0x0
   amota: version = 0x0
   amota: fwLength = 0x19078
   amota: fwCrc = 0xba2cb55f
   amota: fwStartAddr = 0x4000
   amota: fwDataType = 0x0
   amota: storageType = 0x0
   amota: imageId = 0xffffffff
   amota: ============= fw header end ===============
   amota: Packet received correctly, OTA is ongoing...
   amota: Packet received correctly, OTA is ongoing...
   amota: Packet received correctly, OTA is ongoing...
   ...
   amota: CRC verification succeeds
   amota: OTA downloading finished, will disconnect BLE link soon
