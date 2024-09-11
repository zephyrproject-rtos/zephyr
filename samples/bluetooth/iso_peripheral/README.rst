.. zephyr:code-sample:: ble_peripheral_iso
   :name: ISO (Peripheral)
   :relevant-api: bt_bas bluetooth

   Implement a Bluetooth LE Peripheral that uses isochronous channels.

Overview
********

This sample demonstrates how to use isochronous channels as a peripheral.
The sample starts advertising, waits for a central to connect to it and set up an isochronous channel.
Once the isochronous channel is set up, received isochronous data is printed out.
It is recommended to run this sample together with the :zephyr:code-sample:`ble_central_iso` sample.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support
* A Bluetooth Controller and board that supports setting
  CONFIG_BT_CTLR_PERIPHERAL_ISO=y

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/iso_peripheral` in the Zephyr tree.

1. Start the application.
   In the terminal window, check that it is advertising.

      Bluetooth initialized
      Advertising successfully started

2. Observe that the central device connects and sets up an isochronous channel.

      Connected E8:DC:8D:B3:47:69 (random)
      Incoming request from 0x20002260
      ISO Channel 0x20000698 connected

3. Observe that incoming data is printed.

      Incoming data channel 0x20000698 len 1
               00
      Incoming data channel 0x20000698 len 2
               0001
      Incoming data channel 0x20000698 len 3
               000102
      Incoming data channel 0x20000698 len 4
               00010203
      Incoming data channel 0x20000698 len 5
               0001020304
      Incoming data channel 0x20000698 len 6
               000102030405
      Incoming data channel 0x20000698 len 7
               000102...040506
      Incoming data channel 0x20000698 len 8
               000102...050607
      Incoming data channel 0x20000698 len 9
               000102...060708
      Incoming data channel 0x20000698 len 10
               000102...070809
      Incoming data channel 0x20000698 len 11
               000102...08090a
      Incoming data channel 0x20000698 len 12
               000102...090a0b

See :ref:`bluetooth samples section <bluetooth-samples>` for more details.
