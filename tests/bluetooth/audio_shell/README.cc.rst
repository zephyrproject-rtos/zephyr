Call Control for Generic Audio Content Control
##############################################

This document describes how to run the call control functionality, both as
a client and as a (telephone bearer service (TBS)) server. Note that in the
examples below, some lines of debug have been removed to make this shorter
and provide a better overview.

Call Control Client (CCP)
*************************

The call control client will typically exist on a resource restricted device,
such as headphones, but may also exist on e.g. phones or laptops. The call
control client will also thus typically be the advertiser. The client can
control the states of calls on a server using the call control point.

It is necessary to have :code:`BT_DEBUG_CCP` enabled for using the CCP client
interactively.

Using the call control client
=============================

When the btaudio stack has been initialized (:code:`btaudio init`),
and a device has been connected, the call control client can discover TBS on the
connected device calling :code:`btaudio ccp discover`, which will start a
discovery for the TBS UUIDs and store the handles, and optionally subscribe to
all notifications (default is to subscribe to all).

Since a server may have multiple TBS instances, most of the ccp commands will
take an index (starting from 0) as input. Joining calls require at least 2 call
IDs, and all call indexes shall be on the same TBS instance.

A server may also have a GTBS instance, which is an abstraction layer for all
the telephone bearers on the server. If the server has both GTBS and TBS,
the client may subscribe and use either when sending requests if
:code:`BT_CCP_GTBS` is enabled.

In the following examples, notifications from GTBS is ignored, unless otherwise
specified.

Example usage
=============

Setup
-----

.. code-block:: console

   uart:~$ btaudio init
   uart:~$ bt advertise on
   Advertising started

When connected
--------------

Placing a call:

.. code-block:: console

   uart:~$ btaudio ccp discover
   <dbg> bt_ccp.primary_discover_func: Discover complete, found 1 instances (GTBS found)
   <dbg> bt_ccp.discover_func: Setup complete for 1 / 1 TBS
   <dbg> bt_ccp.discover_func: Setup complete GTBS
   uart:~$ btaudio ccp originate 0 tel:123
   <dbg> bt_ccp.notify_handler: Index 0
   <dbg> bt_ccp.current_calls_notify_handler: Call 0x01 is in the dialing state with URI tel:123
   <dbg> bt_ccp.call_cp_notify_handler: Status: success for the originate opcode for call 0x00
   <dbg> bt_ccp.notify_handler: Index 0
   <dbg> bt_ccp.current_calls_notify_handler: Call 0x01 is in the alerting state with URI tel:123
   <call answered by peer device, and status notified by TBS server>
   <dbg> bt_ccp.notify_handler: Index 0
   <dbg> bt_ccp.current_calls_notify_handler: Call 0x01 is in the active state with URI tel:123

Placing a call on GTBS:

.. code-block:: console

   uart:~$ btaudio ccp originate 0 tel:123
   <dbg> bt_ccp.notify_handler: Index 0
   <dbg> bt_ccp.current_calls_notify_handler: Call 0x01 is in the dialing state with URI tel:123
   <dbg> bt_ccp.call_cp_notify_handler: Status: success for the originate opcode for call 0x00
   <dbg> bt_ccp.notify_handler: Index 0
   <dbg> bt_ccp.current_calls_notify_handler: Call 0x01 is in the alerting state with URI tel:123
   <call answered by peer device, and status notified by TBS server>
   <dbg> bt_ccp.notify_handler: Index 0
   <dbg> bt_ccp.current_calls_notify_handler: Call 0x01 is in the active state with URI tel:123

It is necessary to set an outgoing caller ID before placing a call.

Accepting incoming call from peer device:

.. code-block:: console
   <dbg> bt_ccp.incoming_uri_notify_handler: tel:123
   <dbg> bt_ccp.in_call_notify_handler: tel:456
   <dbg> bt_ccp.friendly_name_notify_handler: Peter
   <dbg> bt_ccp.current_calls_notify_handler: Call 0x05 is in the incoming state with URI tel:456
   uart:~$ btaudio ccp accept 0 5
   <dbg> bt_ccp.call_cp_callback_handler: Status: success for the accept opcode for call 0x05
   <dbg> bt_ccp.current_calls_notify_handler: Call 0x05 is in the active state with URI tel


Terminate call:

.. code-block:: console
   uart:~$ btaudio ccp terminate 0 5
   <dbg> bt_ccp.termination_reason_notify_handler: ID 0x05, reason 0x06
   <dbg> bt_ccp.call_cp_notify_handler: Status: success for the terminate opcode for call 0x05
   <dbg> bt_ccp.current_calls_notify_handler:

Telephone Bearer Service (TBS)
******************************
The telephone bearer service is a service that typically resides on devices that
can make calls, including calls from apps such as Skype, e.g. (smart)phones and
PCs.

It is necessary to have :code:`BT_DEBUG_TBS` enabled for using the TBS server
interactively.

Using the telephone bearer service
==================================
TBS can be controlled locally, or by a remote device (when in a call). For
example a remote device may initiate a call to the device with the TBS server,
or the TBS server may initiate a call to remote device, without a CCP client.
The TBS implementation is capable of fully controlling any call.

Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ btaudio init
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

When connected
--------------

Answering a call for a peer device originated by a client:

.. code-block:: console

   <dbg> bt_tbs.write_call_cp: Index 0: Processing the originate opcode
   <dbg> bt_tbs.originate_call: New call with call index 1
   <dbg> bt_tbs.write_call_cp: Index 0: Processed the originate opcode with status success for call index 1
   uart:~$ btaudio tbs remote_answer 1
   TBS succeeded for call_id: 1

Incoming call from a peer device, accepted by client:

.. code-block:: console

   uart:~$ btaudio tbs incoming 0 tel:123 tel:456 Peter
   TBS succeeded for call_id: 4
   <dbg> bt_tbs.bt_tbs_remote_incoming: New call with call index 4
   <dbg> bt_tbs.write_call_cp: Index 0: Processed the accept opcode with status success for call index 4
