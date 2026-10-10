.. zephyr:code-sample:: mfrc522
   :name: MFRC522

   Use MFRC522 device driver to detect RFID tags.

Overview
********

This sample shows how to use the MFRC522 driver API to poll for RFID
tags. On detection, the tag UID is logged to the console. The commands
sent use the ISO/IEC 14443A protocol. The driver communicates to MFRC522
device via SPI, and requires gpio configured for hard reset in device
tree overlay.

Building and Running
********************
Build the application for the :zephyr:board:`nucleo_f401re` board.

.. zephyr-app-commands::
   :zephyr-app: samples/rfid/mfrc522
   :board: nucleo_f401re
   :goals: build flash

Sample output
*************

.. code-block:: console

   [00:00:00.001,000] <inf> mfrc522: timeout set to 25000 us
   [00:00:00.001,000] <inf> mfrc522: chip type 0x9 version 2
   *** Booting Zephyr OS build cdba7c81c2ba ***
   [00:00:00.001,000] <inf> mfrc522: enabling rf
   [00:00:00.001,000] <inf> mfrc522_sample: starting poll for rfid tags
   [00:00:04.415,000] <inf> mfrc522_sample: UID
                                          2b 36 8d 32 a2                                   |+6.2.
   [00:00:08.802,000] <inf> mfrc522_sample: UID
                                          3a 69 3d c1 af                                   |:i=..
