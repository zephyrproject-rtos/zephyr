Bluetooth: Basic Audio Profile: Broadcast Assistant Shell
#########################################################

This document describes how to run the BAP Broadcast Assistant functionality.
Note that in the examples below, some lines of debug have been
removed to make this shorter and provide a better overview.

The Broadcast Assistant is responsible for offloading scan for a resource
restricted device, such that scanning does not drain the battery. The Broadcast
Assistant shall support scanning for periodic advertisements and may optionally
support the periodic advertisements synchronization transfer (PAST) protocol.

The Broadcast Assistant will typically be phones or laptops.
The Broadcast Assistant scans for periodic advertisements and transfer
information to the server.

It is necessary to have
:kconfig:option:`CONFIG_BT_BAP_BROADCAST_ASSISTANT_LOG_LEVEL_DBG` enabled for
using the Broadcast Assistant interactively.

When the Bluetooth stack has been initialized (:code:`bt init`),
and a device has been connected, the Broadcast Assistant can discover BASS on
the connected device calling :code:`bap_broadcast_assistant discover`, which
will start a discovery for the BASS UUIDs and store the handles, and
subscribe to all notifications.

.. code-block:: console

   uart:~$ bap_broadcast_assistant --help
   bap_broadcast_assistant - Bluetooth BAP broadcast assistant client shell
                           commands
   Subcommands:
   discover          : Discover BASS on the server
   scan_start        : Start scanning for broadcasters
   scan_stop         : Stop scanning for BISs
   add_src           : Add a source <address: XX:XX:XX:XX:XX:XX> <type:
                        public/random> <adv_sid> <sync_pa> <broadcast_id>
                        [<pa_interval>] [<sync_bis>] [<metadata>]
   add_broadcast_id  : Add a source by broadcast ID <broadcast_id> <sync_pa>
                        [<sync_bis>] [<metadata>]
   add_pa_sync       : Add a PA sync as a source <sync_pa> <broadcast_id>
                        [bis_index [bis_index [bix_index [...]]]]>
   mod_src           : Set sync <src_id> <sync_pa> [<pa_interval> | "unknown"] [<sync_bis>]
                        [<metadata>]
   broadcast_code    : Send a string-based broadcast code of up to 16 bytes
                        <src_id> <broadcast code>
   rem_src           : Remove a source <src_id>
   read_state        : Remove a source <index>

Example usage
*************

Setup
=====

.. code-block:: console

   uart:~$ bt init
   uart:~$ bap init
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

When connected
==============

Start scanning for periodic advertisements for a server:
--------------------------------------------------------

.. note::
   The Broadcast Assistant will not actually start scanning for periodic
   advertisements, as that feature is still, at the time of writing, not
   implemented.

.. code-block:: console

   uart:~$ bap_broadcast_assistant discover
   BASS discover done with 1 recv states
   uart:~$ bap_broadcast_assistant scan_start true
   BASS scan start successful
   Found broadcaster with ID 0x05BD38 and addr 1E:4D:0A:AA:6E:49 (random) and sid 0x00

Adding a source to the receive state with add_src:
--------------------------------------------------

.. code-block:: console

   uart:~$ bap_broadcast_assistant add_src 11:22:33:44:55:66 public 5 1 1
   BASS recv state: src_id 0, addr 11:22:33:44:55:66 (public), sid 5, sync_state 1, encrypt_state 000000000000000000000000000000000
        [0]: BIS sync 0, metadata_len 0


Adding a source to the receive state with add_broadcast_id (recommended):
-------------------------------------------------------------------------

.. code-block:: console

   uart:~$ bap_broadcast_assistant add_broadcast_id 0x05BD38 true
   [DEVICE]: 1E:4D:0A:AA:6E:49 (random), AD evt type 5, RSSI -28 Broadcast Audio Source C:0 S:0 D:0 SR:0 E:1 Prim: LE 1M, Secn: LE 2M, Interval: 0x03c0 (1200000 us), SID: 0x0
   Found BAP broadcast source with address 1E:4D:0A:AA:6E:49 (random) and ID 0x05BD38
   BASS recv state: src_id 0, addr 1E:4D:0A:AA:6E:49 (random), sid 0, sync_state 0, encrypt_state 0
         [0]: BIS sync 0x0000, metadata_len 0
   BASS add source successful
   BASS recv state: src_id 0, addr 1E:4D:0A:AA:6E:49 (random), sid 0, sync_state 2, encrypt_state 0
         [0]: BIS sync 0x0000, metadata_len 0
   BASS recv state: src_id 0, addr 1E:4D:0A:AA:6E:49 (random), sid 0, sync_state 2, encrypt_state 0
         [0]: BIS sync 0x0000, metadata_len 4
                  Metadata length 2, type 2, data: 0100


Modifying a receive state:
--------------------------

.. code-block:: console

   uart:~$ bap_broadcast_assistant mod_src 0 true 0x03c0 0x02
   BASS modify source successful
   BASS recv state: src_id 0, addr 1E:4D:0A:AA:6E:49 (random), sid 0, sync_state 2, encrypt_state 0
         [0]: BIS sync 0x0001, metadata_len 4
                  Metadata length 2, type 2, data: 0100

Supplying a broadcast code:
---------------------------

.. code-block:: console

   uart:~$ bap_broadcast_assistant broadcast_code 0 secretCode
   Sending broadcast code:
   00000000: 73 65 63 72 65 74 43 6f 64 65 00 00 00 00 00 00 |secretCo de....|
   uart:~$ BASS broadcast code successful
