Bluetooth: Broadcast Audio Profile Broadcast Assistant
######################################################

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

It is necessary to have :code:`BT_DEBUG_BAP_BROADCAST_ASSISTANT` enabled for
using the Broadcast Assistant interactively.

When the Bluetooth stack has been initialized (:code:`bt init`),
and a device has been connected, the Broadcast Assistant can discover BASS on
the connected device calling :code:`bap_broadcast_assistant discover`, which
will start a discovery for the BASS UUIDs and store the handles, and
subscribe to all notifications.

.. code-block:: console

   bap_broadcast_assistant - Bluetooth BAP Broadcast Assistant shell commands
   Subcommands:
      discover        :Discover BASS on the server
      scan_start      :Start scanning for broadcasters
      scan_stop       :Stop scanning for BISs
      add_src         :Add a source <address: XX:XX:XX:XX:XX:XX> <type:
                       public/random> <adv_sid> <sync_pa> <broadcast_id>
                       [<pa_interval>] [<sync_bis>] [<metadata>]
      mod_src         :Set sync <src_id> <sync_pa> [<pa_interval>] [<sync_bis>]
                       [<metadata>]
      broadcast_code  :Send a space separated broadcast code of up to 16 bytes
                       <src_id> [broadcast code]
      rem_src         :Remove a source <src_id>
      read_state      :Remove a source <index>

Example usage
*************

Setup
=====

.. code-block:: console

   uart:~$ bt init
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

When connected
==============

Start scanning for periodic advertisements for a server:

.. note::
   The Broadcast Assistant will not actually start scanning for periodic
   advertisements, as that feature is still, at the time of writing, not
   implemented.

.. code-block:: console

   uart:~$ bap_broadcast_assistant discover
   <dbg> bt_bap_broadcast_assistant.char_discover_func: Found 1 BASS receive states
   <dbg> bt_bap_broadcast_assistant.read_recv_state_cb: src_id 0, PA 0, BIS 0, encrypt 0, addr 00:00:00:00:00:00 (public), sid 0, metadata_len 0
   uart:~$ bap_broadcast_assistant scan_start
   <dbg> bt_bap_broadcast_assistant.write_func: err: 0x00, handle 0x001e

Adding a source to the receive state:

.. code-block:: console

   uart:~$ bap_broadcast_assistant add_src 11:22:33:44:55:66 public 5 1 1
   BASS recv state: src_id 0, addr 11:22:33:44:55:66 (public), sid 5, sync_state 1, encrypt_state 000000000000000000000000000000000
        [0]: BIS sync 0, metadata_len 0
