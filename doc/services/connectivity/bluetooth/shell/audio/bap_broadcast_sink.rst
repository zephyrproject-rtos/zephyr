Bluetooth: Basic Audio Profile: Broadcast Sink Shell
####################################################

This document describes how to run the Broadcast Sink functionality.
Note that in the examples below, some lines of debug have been
removed to make this shorter and provide a better overview.


.. code-block:: console

   uart:~$ bap_broadcast_sink --help
   bap_broadcast_sink - Bluetooth BAP Broadcast Sink shell commands
   Subcommands:
     init            : Initialize and register callbacks
     create          : Create the broadcast sink by Broadcast ID: 0x<broadcast_id>
     create_by_name  : Create the broadcast sink by Broadcast Name:
                       <broadcast_name>
     sync            : Sync to one of my BIS indexes: 0x<bis_index>
                       [[[0x<bis_index>] 0x<bis_index>] ...] [bcode <broadcast
                       code> || bcode_str <broadcast code as string>]
     stop            : Stop the broadcast sink
     term            : Delete the broadcast sink


Example Usage
*************

Scan for and establish a broadcast sink stream.
The command :code:`bap_broadcast_sink create` will either use existing periodic advertising
sync (if exist) or start scanning and sync to the periodic advertising with the provided broadcast
ID before syncing to the BIG.

.. code-block:: console

   uart:~$ bap_broadcast_sink init
   uart:~$ bap_broadcast_sink create 0xEF6716
   No PA sync available, starting scanning for broadcast_id
   Found broadcaster with ID 0xEF6716 and addr 03:47:95:75:C0:08 (random) and sid 0x00
   Attempting to PA sync to the broadcaster
   PA synced to broadcast with broadcast ID 0xEF6716
   Attempting to sync to the BIG
   Received BASE from sink 0x20019080:
   Presentation delay: 40000
   Subgroup count: 1
   Subgroup 0x20024182:
      Codec Format: 0x06
      Company ID  : 0x0000
      Vendor ID   : 0x0000
      codec cfg id 0x06 cid 0x0000 vid 0x0000 count 16
         Codec specific configuration:
         Sampling frequency: 16000 Hz (3)
         Frame duration: 10000 us (1)
         Channel allocation:
                  Front left (0x00000001)
                  Front right (0x00000002)
         Octets per codec frame: 40
         Codec specific metadata:
         Streaming audio contexts:
            Unspecified (0x0001)
         BIS index: 0x01
            codec cfg id 0x06 cid 0x0000 vid 0x0000 count 6
            Codec specific configuration:
               Channel allocation:
                  Front left (0x00000001)
            Codec specific metadata:
               None
         BIS index: 0x02
            codec cfg id 0x06 cid 0x0000 vid 0x0000 count 6
            Codec specific configuration:
               Channel allocation:
                  Front right (0x00000002)
            Codec specific metadata:
               None
   Possible indexes: 0x01 0x02
   Sink 0x20019110 is ready to sync without encryption
   uart:~$ bap_broadcast_sink sync 0x01


Scan for and establish a broadcast sink stream by broadcast name
----------------------------------------------------------------

The command :code:`bap_broadcast_sink create_by_name` will start scanning and sync to the periodic
advertising with the provided broadcast name before syncing to the BIG.

.. code-block:: console

   uart:~$ bap_broadcast_sink init
   uart:~$ bap_broadcast_sink create_by_name "Test Broadcast"
   Starting scanning for broadcast_name
   Found matched broadcast name 'Test Broadcast' with address 03:47:95:75:C0:08 (random)
   Found broadcaster with ID 0xEF6716 and addr 03:47:95:75:C0:08 (random) and sid 0x00
   Attempting to PA sync to the broadcaster
   PA synced to broadcast with broadcast ID 0xEF6716
   Attempting to create the sink
   Received BASE from sink 0x20019080:
   Presentation delay: 40000
   Subgroup count: 1
   Subgroup 0x20024182:
      Codec Format: 0x06
      Company ID  : 0x0000
      Vendor ID   : 0x0000
      codec cfg id 0x06 cid 0x0000 vid 0x0000 count 16
         Codec specific configuration:
         Sampling frequency: 16000 Hz (3)
         Frame duration: 10000 us (1)
         Channel allocation:
                  Front left (0x00000001)
                  Front right (0x00000002)
         Octets per codec frame: 40
         Codec specific metadata:
         Streaming audio contexts:
            Unspecified (0x0001)
         BIS index: 0x01
            codec cfg id 0x06 cid 0x0000 vid 0x0000 count 6
            Codec specific configuration:
               Channel allocation:
                  Front left (0x00000001)
            Codec specific metadata:
               None
         BIS index: 0x02
            codec cfg id 0x06 cid 0x0000 vid 0x0000 count 6
            Codec specific configuration:
               Channel allocation:
                  Front right (0x00000002)
            Codec specific metadata:
               None
   Possible indexes: 0x01 0x02
   Sink 0x20019110 is ready to sync without encryption
   uart:~$ bap_broadcast_sink sync 0x01

Syncing to encrypted broadcast
------------------------------

If the broadcast is encrypted, the broadcast code can be entered with the
:code:`bap_broadcast_sink sync` command as such:

.. code-block:: console

   Sink 0x20019110 is ready to sync with encryption
   uart:~$ bap_broadcast_sink sync 0x01 bcode 0102030405060708090a0b0c0d0e0f

The broadcast code can be 1-16 values, either as a string or a hexadecimal value.

.. code-block:: console

   Sink 0x20019110 is ready to sync with encryption
   uart:~$ bap_broadcast_sink sync 0x01 bcode_str thisismycode

Stop and release a broadcast sink stream:

.. code-block:: console

   uart:~$ bap_broadcast_sink stop
   uart:~$ bap_broadcast_sink term
