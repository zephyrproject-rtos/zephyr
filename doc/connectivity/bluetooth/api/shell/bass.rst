Bluetooth: Broadcast Audio Scan Service
#######################################

This document describes how to run the BASS functionality, both as a client
and as a server. Note that in the examples below, some lines of debug have been
removed to make this shorter and provide a better overview.

The client is responsible for offloading scan for a resource restricted device,
such that scanning does not drain the battery. The client shall support scanning
for periodic advertisements, and the server shall support synchronizing to
periodic advertisements. They may both optionally support the
periodic advertisements synchronization transfer (PAST) protocol.

Broadcast Audio Scan Client (Scan Offloader)
********************************************

The BASS client will typically be phones or laptops.
The client scans for periodic advertisements and transfer information to the
server.

It is necessary to have :code:`BT_DEBUG_BASS_CLIENT` enabled for using the BASS
client interactively.

Using the BASS client
=====================

When the Bluetooth stack has been initialized (:code:`bt init`),
and a device has been connected, the BASS client can discover BASS on
the connected device calling :code:`bass_client discover`, which will
start a discovery for the BASS UUIDs and store the handles, and optionally
subscribe to all notifications (default is to subscribe to all).

Since a server may have multiple included Volume Offset or Audio Input service
instances, some of the client commands commands will take an index
(starting from 0) as input.

.. code-block:: console

   bass_client - Bluetooth BASS client shell commands
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
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

When connected
--------------

Start scanning for periodic advertisements for a server:

.. note::
   The BASS client will not actually start scanning for periodic advertisements,
   as that feature is still, at the time of writing, not implemented.

.. code-block:: console

   uart:~$ bass_client discover
   <dbg> bt_bass_client.char_discover_func: Found 1 BASS receive states
   <dbg> bt_bass_client.read_recv_state_cb: src_id 0, PA 0, BIS 0, encrypt 0, addr 00:00:00:00:00:00 (public), sid 0, metadata_len 0
   uart:~$ bass_client scan_start
   <dbg> bt_bass_client.write_func: err: 0x00, handle 0x001e

Adding a source to the receive state:

.. code-block:: console

   uart:~$ bass_client add_src 11:22:33:44:55:66 public 5 1 1
   BASS recv state: src_id 0, addr 11:22:33:44:55:66 (public), sid 5, sync_state 1, encrypt_state 000000000000000000000000000000000
        [0]: BIS sync 0, metadata_len 0

BASS Server
***********
The BASS server typically resides on devices that have inputs or outputs.

It is necessary to have :code:`BT_DEBUG_BASS` enabled for using the BASS server
interactively.

Using the BASS server
================================
The BASS server can currently only set the sync state of a receive state, but
does not actually support syncing with periodic advertisements yet.

.. code-block:: console

   bass --help
   bass - Bluetooth BASS shell commands
   Subcommands:
      init    :Initialize the service and register callbacks
      synced  :Set server scan state <src_id> <pa_synced> <bis_syncs> <enc_state>


Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   uart:~$ bt advertise on
   Advertising started

When connected
--------------

Set sync state for a source:

.. code-block:: console

   uart:~$ bass synced 0 1 3 0
