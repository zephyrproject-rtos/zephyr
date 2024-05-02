Bluetooth: Call Control Profile
###############################

This document describes how to run the call control functionality, both as
a client and as a (telephone bearer service (TBS)) server. Note that in the
examples below, some lines of debug have been removed to make this shorter
and provide a better overview.

Telephone Bearer Service Client
*******************************

The telephone bearer service client will typically exist on a resource
restricted device, such as headphones, but may also exist on e.g. phones or
laptops. The call control client will also thus typically be the advertiser.
The client can control the states of calls on a server using the call control
point.

It is necessary to have :kconfig:option:`CONFIG_BT_TBS_CLIENT_LOG_LEVEL_DBG`
enabled for using the client interactively.

Using the telephone bearer service client
=========================================

When the Bluetooth stack has been initialized (:code:`bt init`),
and a device has been connected, the telephone bearer service client can
discover TBS on the connected device calling :code:`tbs_client discover`, which
will start a discovery for the TBS UUIDs and store the handles, and optionally
subscribe to all notifications (default is to subscribe to all).

Since a server may have multiple TBS instances, most of the tbs_client commands
will take an index (starting from 0) as input. Joining calls require at least 2
call IDs, and all call indexes shall be on the same TBS instance.

A server may also have a GTBS instance, which is an abstraction layer for all
the telephone bearers on the server. If the server has both GTBS and TBS,
the client may subscribe and use either when sending requests if
:code:`BT_TBS_CLIENT_GTBS` is enabled.

.. code-block:: console

   tbs_client --help
   tbs_client - Bluetooth TBS_CLIENT shell commands
   Subcommands:
      discover                       :Discover TBS [subscribe]
      set_signal_reporting_interval  :Set the signal reporting interval
                                       [<{instance_index, gtbs}>] <interval>
      originate                      :Originate a call [<{instance_index, gtbs}>]
                                       <uri>
      terminate                      :terminate a call [<{instance_index, gtbs}>]
                                       <id>
      accept                         :Accept a call [<{instance_index, gtbs}>] <id>
      hold                           :Place a call on hold [<{instance_index,
                                       gtbs}>] <id>
      retrieve                       :Retrieve a held call [<{instance_index,
                                       gtbs}>] <id>
      read_provider_name             :Read the bearer name [<{instance_index,
                                       gtbs}>]
      read_bearer_uci                :Read the bearer UCI [<{instance_index, gtbs}>]
      read_technology                :Read the bearer technology [<{instance_index,
                                       gtbs}>]
      read_uri_list                  :Read the bearer's supported URI list
                                       [<{instance_index, gtbs}>]
      read_signal_strength           :Read the bearer signal strength
                                       [<{instance_index, gtbs}>]
      read_signal_interval           :Read the bearer signal strength reporting
                                       interval [<{instance_index, gtbs}>]
      read_current_calls             :Read the current calls [<{instance_index,
                                       gtbs}>]
      read_ccid                      :Read the CCID [<{instance_index, gtbs}>]
      read_status_flags              :Read the in feature and status value
                                       [<{instance_index, gtbs}>]
      read_uri                       :Read the incoming call target URI
                                       [<{instance_index, gtbs}>]
      read_call_state                :Read the call state [<{instance_index, gtbs}>]
      read_remote_uri                :Read the incoming remote URI
                                       [<{instance_index, gtbs}>]
      read_friendly_name             :Read the friendly name of an incoming call
                                       [<{instance_index, gtbs}>]
      read_optional_opcodes          :Read the optional opcodes [<{instance_index,
                                       gtbs}>]


In the following examples, notifications from GTBS is ignored, unless otherwise
specified.

Example usage
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   uart:~$ bt advertise on
   Advertising started

When connected
--------------

Placing a call:

.. code-block:: console

   uart:~$ tbs_client discover
   <dbg> bt_tbs_client.primary_discover_func: Discover complete, found 1 instances (GTBS found)
   <dbg> bt_tbs_client.discover_func: Setup complete for 1 / 1 TBS
   <dbg> bt_tbs_client.discover_func: Setup complete GTBS
   uart:~$ tbs_client originate 0 tel:123
   <dbg> bt_tbs_client.notify_handler: Index 0
   <dbg> bt_tbs_client.current_calls_notify_handler: Call 0x01 is in the dialing state with URI tel:123
   <dbg> bt_tbs_client.call_cp_notify_handler: Status: success for the originate opcode for call 0x00
   <dbg> bt_tbs_client.notify_handler: Index 0
   <dbg> bt_tbs_client.current_calls_notify_handler: Call 0x01 is in the alerting state with URI tel:123
   <call answered by peer device, and status notified by TBS server>
   <dbg> bt_tbs_client.notify_handler: Index 0
   <dbg> bt_tbs_client.current_calls_notify_handler: Call 0x01 is in the active state with URI tel:123

Placing a call on GTBS:

.. code-block:: console

   uart:~$ tbs_client originate 0 tel:123
   <dbg> bt_tbs_client.notify_handler: Index 0
   <dbg> bt_tbs_client.current_calls_notify_handler: Call 0x01 is in the dialing state with URI tel:123
   <dbg> bt_tbs_client.call_cp_notify_handler: Status: success for the originate opcode for call 0x00
   <dbg> bt_tbs_client.notify_handler: Index 0
   <dbg> bt_tbs_client.current_calls_notify_handler: Call 0x01 is in the alerting state with URI tel:123
   <call answered by peer device, and status notified by TBS server>
   <dbg> bt_tbs_client.notify_handler: Index 0
   <dbg> bt_tbs_client.current_calls_notify_handler: Call 0x01 is in the active state with URI tel:123

It is necessary to set an outgoing caller ID before placing a call.

Accepting incoming call from peer device:

.. code-block:: console

   <dbg> bt_tbs_client.incoming_uri_notify_handler: tel:123
   <dbg> bt_tbs_client.in_call_notify_handler: tel:456
   <dbg> bt_tbs_client.friendly_name_notify_handler: Peter
   <dbg> bt_tbs_client.current_calls_notify_handler: Call 0x05 is in the incoming state with URI tel:456
   uart:~$ tbs_client accept 0 5
   <dbg> bt_tbs_client.call_cp_callback_handler: Status: success for the accept opcode for call 0x05
   <dbg> bt_tbs_client.current_calls_notify_handler: Call 0x05 is in the active state with URI tel


Terminate call:

.. code-block:: console

   uart:~$ tbs_client terminate 0 5
   <dbg> bt_tbs_client.termination_reason_notify_handler: ID 0x05, reason 0x06
   <dbg> bt_tbs_client.call_cp_notify_handler: Status: success for the terminate opcode for call 0x05
   <dbg> bt_tbs_client.current_calls_notify_handler:

Telephone Bearer Service (TBS)
******************************
The telephone bearer service is a service that typically resides on devices that
can make calls, including calls from apps such as Skype, e.g. (smart)phones and
PCs.

It is necessary to have :kconfig:option:`CONFIG_BT_TBS_LOG_LEVEL_DBG` enabled
for using the TBS server interactively.

Using the telephone bearer service
==================================
TBS can be controlled locally, or by a remote device (when in a call). For
example a remote device may initiate a call to the device with the TBS server,
or the TBS server may initiate a call to remote device, without a TBS_CLIENT client.
The TBS implementation is capable of fully controlling any call.

.. code-block:: console

   tbs --help
   tbs - Bluetooth TBS shell commands
   Subcommands:
      init                        :Initialize TBS
      authorize                   :Authorize the current connection
      accept                      :Accept call <call_index>
      terminate                   :Terminate call <call_index>
      hold                        :Hold call <call_index>
      retrieve                    :Retrieve call <call_index>
      originate                   :Originate call [<instance_index>] <uri>
      join                        :Join calls <id> <id> [<id> [<id> [...]]]
      incoming                    :Simulate incoming remote call [<{instance_index,
                                    gtbs}>] <local_uri> <remote_uri>
                                    <remote_friendly_name>
      remote_answer               :Simulate remote answer outgoing call <call_index>
      remote_retrieve             :Simulate remote retrieve <call_index>
      remote_terminate            :Simulate remote terminate <call_index>
      remote_hold                 :Simulate remote hold <call_index>
      set_bearer_provider_name    :Set the bearer provider name [<{instance_index,
                                    gtbs}>] <name>
      set_bearer_technology       :Set the bearer technology [<{instance_index,
                                    gtbs}>] <technology>
      set_bearer_signal_strength  :Set the bearer signal strength [<{instance_index,
                                    gtbs}>] <strength>
      set_status_flags            :Set the bearer feature and status value
                                    [<{instance_index, gtbs}>] <feature_and_status>
      set_uri_scheme              :Set the URI prefix list <bearer_idx> <uri1 [uri2
                                    [uri3 [...]]]>
      print_calls                 :Output all calls in the debug log

Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

When connected
--------------

Answering a call for a peer device originated by a client:

.. code-block:: console

   <dbg> bt_tbs.write_call_cp: Index 0: Processing the originate opcode
   <dbg> bt_tbs.originate_call: New call with call index 1
   <dbg> bt_tbs.write_call_cp: Index 0: Processed the originate opcode with status success for call index 1
   uart:~$ tbs remote_answer 1
   TBS succeeded for call_id: 1

Incoming call from a peer device, accepted by client:

.. code-block:: console

   uart:~$ tbs incoming 0 tel:123 tel:456 Peter
   TBS succeeded for call_id: 4
   <dbg> bt_tbs.bt_tbs_remote_incoming: New call with call index 4
   <dbg> bt_tbs.write_call_cp: Index 0: Processed the accept opcode with status success for call index 4
