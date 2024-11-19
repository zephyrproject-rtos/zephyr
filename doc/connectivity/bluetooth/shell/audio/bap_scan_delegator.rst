Bluetooth: Broadcast Audio Profile Scan Delegator
#################################################

This document describes how to run the Scan Delegator functionality, Note that
in the examples below, some lines of debug have been
removed to make this shorter and provide a better overview.

The Scan Delegator may optionally support the periodic advertisements
synchronization transfer (PAST) protocol.

The Scan Delegator server typically resides on devices that have inputs or
outputs.

It is necessary to have
:kconfig:option:`CONFIG_BT_BAP_SCAN_DELEGATOR_LOG_LEVEL_DBG` enabled for using
the Scan Delegator interactively.

The Scan Delegator can currently only set the sync state of a receive state, but
does not actually support syncing with periodic advertisements yet.

.. code-block:: console

   bap_scan_delegator --help
   bap_scan_delegator - Bluetooth BAP Scan Delegator shell commands
   Subcommands:
     init                : Initialize the service and register callbacks
     set_past_pref       : Set PAST preference <true || false>
     sync_pa             : Sync to PA <src_id>
     term_pa             : Terminate PA sync <src_id>
     add_src             : Add a PA as source <addr> <sid> <broadcast_id>
                          <enc_state> [bis_sync [metadata]]
     add_src_by_pa_sync  : Add a PA as source <broadcast_id> <enc_state> [bis_sync
                          [metadata]]
     mod_src             : Modify source <src_id> <broadcast_id> <enc_state>
                          [bis_sync [metadata]]
     rem_src             : Remove source <src_id>
     synced              : Set server scan state <src_id> <bis_syncs>




Example Usage
*************

Setup
=====

.. code-block:: console

   uart:~$ bt init
   uart:~$ bap_scan_delegator init
   uart:~$ bt advertise on
   Advertising started

Adding a source
===============

.. code-block:: console

   uart:~$ bap_scan_delegator add_src 11:22:33:44:55:66 public 0 1234 0
   Receive state with ID 0 updated

Adding a source from a PA sync
==============================

.. code-block:: console

   uart:~$ bt scan on
   Found broadcaster with ID 0x681A22 and addr 2C:44:05:82:EB:82 (random) and sid 0x00 (looking for 0x1000000)
   uart:~$ bt scan off
   uart:~$ bt per-adv-sync-create 2C:44:05:82:EB:82 (random) 0
   PA 0x2003e9b0 synced
   uart:~$ bap_scan_delegator add_src_by_pa_sync 0x681A22 0
   Receive state with ID 0 updated

When connected
==============

Set sync state for a source:

.. code-block:: console

   uart:~$ bap_scan_delegator synced 0 1 3 0
