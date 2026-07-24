.. zephyr:code-sample:: rc522
   :name: RC522

   Use RC522 NFC driver to detect RFID tags.

Overview
********

This sample shows how to use the Zephyr NFC API driver to poll for RFID
tags. On detection, the tag UID is logged to the console. The NFC (RFID)
reader device tested is a RC522, and commands are for the MIFARE protocol.

Building and Running
********************
Build the application for the :zephyr:board:`nucleo_f401re` board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/nfc/rc522
   :board: nucleo_f401re
   :goals: build flash

Sample output
*************

.. code-block:: console

   [00:00:00.001,000] <inf> rc522: timeout set to 25000 us
   [00:00:00.001,000] <inf> rc522: chip type 0x9 version 2
   *** Booting Zephyr OS build f7fdc86aa7af ***
   [00:00:00.001,000] <inf> rc522: enabling rf
   [00:00:00.002,000] <inf> rc522_sample: starting poll for nfc tags
   [00:00:14.335,000] <inf> rc522_sample: UID
                                          2b 36 8d 32 a2                                   |+6.2.
   [00:00:19.549,000] <inf> rc522_sample: UID
                                          3a 69 3d c1 af                                   |:i=..
