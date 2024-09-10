.. zephyr:code-sample:: bluetooth_extended_advertising
   :name: Extended Advertising
   :relevant-api: bluetooth

   Use the Bluetooth LE extended advertising feature.

Overview
********

This sample demonstrates the use of the extended advertising feature, by:

- Outlining the steps required to initialize an extended advertising application.
- Demo how to gracefully restart the functionality, after a disconnect.

The sample consists of the advertiser initiating a connectable advertisement set,
which prompts the scanner to connect after scanning for extended advertisements.
Once the connection is established, the advertiser waits for 5 seconds to disconnect.
After the connection is dropped, the advertiser immediately restarts broadcasting,
while the scanner cools-down for 5 seconds to restart its process.

This sample handles all actions in a separate thread, to promote good design
practices. Even though it is not strictly required, scheduling from another context is
strongly recommended (e.g. using a work item), as re-starting an advertiser or
scanner from within the ``recycled`` callback exposes the application to deadlocking.

Requirements
************

* Two boards with Bluetooth Low Energy support

Building and Running
********************

This sample can be found under
:zephyr_file:`samples/bluetooth/extended_adv` in the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.

This sample uses two applications, so two devices need to be setup.
Flash one device with the scanner application, and another device with the
advertiser application.

The two devices should automatically connect if they are close enough.

Here are the outputs you should get by default:

Advertiser:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v3.5.0-4935-gfc7972183da5 ***
        Starting Extended Advertising Demo
        Starting Extended Advertising
        Connected (err 0x00)
        Connected state!
        Initiating disconnect within 5 seconds...
        Disconnected (reason 0x16)
        Connection object available from previous conn. Disconnect is complete!
        Disconnected state! Restarting advertising
        Starting Extended Advertising
        Connected (err 0x00)
        Connected state!
        Initiating disconnect within 5 seconds...
        Disconnected (reason 0x16)
        Connection object available from previous conn. Disconnect is complete!
        Disconnected state! Restarting advertising
        Starting Extended Advertising

Scanner:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v3.5.0-4935-ge3308caf97bc ***
        Starting Extended Advertising Demo [Scanner]
        Found extended advertisement packet!
        Stopping scan
        Connected (err 0x00)
        Connected state!
        Disconnected (reason 0x13)
        Recycled cb called!
        Disconnected, cooldown for 5 seconds!
        Starting to scan for extended adv
        Found extended advertisement packet!
        Stopping scan
        Connected (err 0x00)
        Connected state!
        Disconnected (reason 0x13)
        Recycled cb called!
        Disconnected, cooldown for 5 seconds!
        Starting to scan for extended adv
        Found extended advertisement packet!
        Stopping scan
        Connected (err 0x00)
        Connected state!
        Disconnected (reason 0x13)
        Recycled cb called!
        Disconnected, cooldown for 5 seconds!
