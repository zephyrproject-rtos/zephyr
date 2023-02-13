Bluetooth: Broadcast Audio Profile Scan Delegator
#################################################

This document describes how to run the Scan Delegator functionality, Note that
in the examples below, some lines of debug have been
removed to make this shorter and provide a better overview.

The Scan Delegator may optionally support the periodic advertisements
synchronization transfer (PAST) protocol.

The Scan Delegator server typically resides on devices that have inputs or
outputs.

It is necessary to have :code:`BT_DEBUG_BAP_SCAN_DELEGATOR` enabled for using
the Scan Delegator interactively.

The Scan Delegator can currently only set the sync state of a receive state, but
does not actually support syncing with periodic advertisements yet.

.. code-block:: console

   bap_scan_delegator --help
   bap_scan_delegator - Bluetooth BAP Scan Delegator shell commands
   Subcommands:
      init    :Initialize the service and register callbacks
      synced  :Set server scan state <src_id> <pa_synced> <bis_syncs> <enc_state>


Example Usage
*************

Setup
=====

.. code-block:: console

   uart:~$ bt init
   uart:~$ bt advertise on
   Advertising started

When connected
==============

Set sync state for a source:

.. code-block:: console

   uart:~$ bap_scan_delegator synced 0 1 3 0
