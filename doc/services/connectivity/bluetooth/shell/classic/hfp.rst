Bluetooth: Classic: HFP Shell
###############################

This document describes how to run the Bluetooth Classic HFP functionality.
The :code:`hfp` command exposes the Bluetooth Classic HFP Shell commands.

There are two sub-commands, :code:`hfp hf` and :code:`hfp ag`.

The :code:`hfp hf` is for Hands-Free Profile (HF) functionality, and the :code:`hfp ag` is
for Audio Gateway (AG) functionality.

Commands
********

All commands can only be used after the ACL connection has been established except
:code:`hfp hf reg` and :code:`hfp ag reg`.

The :code:`hfp` commands:

.. code-block:: console

   uart:~$ hfp
   hfp - Bluetooth HFP shell commands
   Subcommands:
     hf  : HFP HF shell commands
     ag  : HFP AG shell commands

The :code:`hfp hf` commands:

.. code-block:: console

   uart:~$ hfp hf
   hf - HFP HF shell commands
   Subcommands:
     reg                          : [none]
     connect                      : <channel>
     disconnect                   : [none]
     sco_disconnect               : [none]
     cli                          : <enable/disable>
     vgm                          : <gain>
     vgs                          : <gain>
     operator                     : [none]
     audio_connect                : [none]
     auto_select_codec            : <enable/disable>
     select_codec                 : Codec ID
     set_codecs                   : Codec ID Map
     accept                       : <call index>
     reject                       : <call index>
     terminate                    : <call index>
     hold_incoming                : <call index>
     query_respond_hold_status    : [none]
     number_call                  : <phone number>
     memory_dial                  : <memory location>
     redial                       : [none]
     turn_off_ecnr                : [none]
     call_waiting_notify          : <enable/disable>
     release_all_held             : [none]
     set_udub                     : [none]
     release_active_accept_other  : [none]
     hold_active_accept_other     : [none]
     join_conversation            : [none]
     explicit_call_transfer       : [none]
     release_specified_call       : <call index>
     private_consultation_mode    : <call index>
     voice_recognition            : <activate/deactivate>
     ready_to_accept_audio        : [none]
     request_phone_number         : [none]
     transmit_dtmf_code           : <call index> <code(set 0-9, #,*,A-D)>
     query_subscriber             : [none]
     indicator_status             : <Activate/deactivate AG indicators bitmap>
     enhanced_safety              : <enable/disable>
     battery                      : <level>

The :code:`hfp ag` commands:

.. code-block:: console

   uart:~$ hfp ag
   ag - HFP AG shell commands
   Subcommands:
     reg                     : [none]
     connect                 : <channel>
     disconnect              : [none]
     sco_disconnect          : [none]
     ongoing_calls           : <yes or no>
     set_ongoing_calls       : <number> <type> <status> <dir> [all]
     remote_incoming         : <number>
     hold_incoming           : <number>
     remote_reject           : <call index>
     remote_accept           : <call index>
     remote_terminate        : <call index>
     remote_ringing          : <call index>
     outgoing                : <number>
     reject                  : <call index>
     accept                  : <call index>
     hold                    : <call index>
     retrieve                : <call index>
     terminate               : <call index>
     vgm                     : <gain>
     vgs                     : <gain>
     operator                : <mode> <operator>
     audio_connect           : <codec id>
     inband_ringtone         : <enable/disable>
     explicit_call_transfer  : [none]
     voice_recognition       : <activate/deactivate>
     vre_state               : <[R-ready][S-send][P-processing]>
     vre_text                : <[R-ready][S-send][P-processing]> <id> <type>
                               <operation> <text string>
     subscriber              : <empty/notempty>
     signal_strength         : <signal strength>
     roaming_status          : <roaming status>
     battery_level           : <battery level>
     service_availability    : <yes/no>
     hf_indicator            : <indicator> <enable/disable>

HFP AG SLC
**********

The :code:`hfp ag` subcommand provides functionality for HFP AG in Bluetooth Classic.

1. Register HFP AG:

.. code-block:: console

   uart:~$ hfp ag reg

2. Connect to HFP HF:

.. code-block:: console

   uart:~$ hfp ag connect 1

3. Connection is established with the HF device:

.. code-block:: console

   Security changed: XX:XX:XX:XX:XX:XX level 2
   AG received codec id bit map 2
   AG connected
   AG received vgm 0
   AG received vgs 0

4. Disconnect from HFP HF:

.. code-block:: console

   uart:~$ hfp ag disconnect

5. Connection is broken:

.. code-block:: console

   AG disconnected


HFP HF SLC
**********

The :code:`hfp hf` subcommand provides functionality for HFP HF in Bluetooth Classic.

1. Register HFP HF:

.. code-block:: console

   uart:~$ hfp hf reg

2. Connect to HFP AG:

.. code-block:: console

   uart:~$ hfp hf connect 2

3. Connection is established with the AG device:

.. code-block:: console

   Security changed: XX:XX:XX:XX:XX:XX level 2
   HF service 0
   HF signal 0
   HF roam 0
   HF battery 0
   HF ring: in-band
   HF connected

4. Disconnect from HFP HF:

.. code-block:: console

   uart:~$ hfp hf disconnect

5. Connection is broken:

.. code-block:: console

   HF disconnected

Call outgoing
*************

Place a call with the Phone number supplied by the AG:

.. tabs::

   .. group-tab:: Outgoing Call Sequence on AG side

      .. code-block:: console

         uart:~$ hfp ag outgoing 123456
         AG outgoing call 0x20007690, number 123456
         AG SCO connected 0x20005248
         AG SCO info:
           SCO handle 0x0008
           SCO air mode 2
           SCO link type 2
         uart:~$ hfp ag remote_ringing 0
         AG call 0x20007690 start ringing mode 1
         uart:~$ hfp ag remote_accept 0
         AG call 0x20007690 accept

   .. group-tab:: Outgoing Call Sequence on HF side

      .. code-block:: console

         uart:~$ hfp hf auto_select_codec enable
         HF call 0x20007408 outgoing
         codec negotiation: 1
         codec auto selected: id 1
         HF SCO connected 0x20005248
         HF SCO info:
           SCO handle 0x0008
           SCO air mode 2
           SCO link type 2
         HF remote call 0x20007408 start ringing
         HF call 0x20007408 accepted

Place a call with the Phone number supplied by the HF:

.. tabs::

   .. group-tab:: Outgoing Call Sequence on AG side

      .. code-block:: console

         uart:~$
         AG number call
         AG outgoing call 0x20007690, number 123456789
         AG SCO connected 0x20005248
         AG SCO info:
           SCO handle 0x0008
           SCO air mode 2
           SCO link type 2
         uart:~$ hfp ag remote_ringing 0
         AG call 0x20007690 start ringing mode 1
         uart:~$ hfp ag remote_accept 0
         AG call 0x20007690 accept

   .. group-tab:: Outgoing Call Sequence on HF side

      .. code-block:: console

         uart:~$ hfp hf auto_select_codec enable
         uart:~$ hfp hf number_call 123456789
         HF start dialing call: err 0
         HF call 0x20007408 outgoing
         codec negotiation: 1
         codec auto selected: id 1
         HF SCO connected 0x20005248
         HF SCO info:
           SCO handle 0x0008
           SCO air mode 2
           SCO link type 2
         HF remote call 0x20007408 start ringing
         HF call 0x20007408 accepted

Call incoming
*************

Answer incoming call from the AG:

.. tabs::

   .. group-tab:: Incoming Call Sequence on AG side

      .. code-block:: console

         uart:~$ hfp ag remote_incoming 123456
         AG incoming call 0x20007690, number 123456
         AG call 0x20007690 start ringing mode 1
         AG SCO connected 0x20005248
         AG SCO info:
           SCO handle 0x0008
           SCO air mode 2
           SCO link type 2
         uart:~$ hfp ag accept 0
         AG call 0x20007690 accept

   .. group-tab:: Incoming Call Sequence on HF side

      .. code-block:: console

         uart:~$ hfp hf auto_select_codec enable
         HF call 0x20007408 incoming
         codec negotiation: 1
         codec auto selected: id 1
         HF SCO connected 0x20005248
         HF SCO info:
           SCO handle 0x0008
           SCO air mode 2
           SCO link type 2
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 accepted

Answer incoming call from the HF:

.. tabs::

   .. group-tab:: Incoming Call Sequence on AG side

      .. code-block:: console

         uart:~$ hfp ag remote_incoming 123456
         AG incoming call 0x20007690, number 123456
         AG codec negotiation result 0
         AG call 0x20007690 start ringing mode 1
         AG SCO connected 0x20005248
         AG SCO info:
           SCO handle 0x0008
           SCO air mode 2
           SCO link type 2
         AG call 0x20007690 accept

   .. group-tab:: Incoming Call Sequence on HF side

      .. code-block:: console

         uart:~$ hfp hf auto_select_codec enable
         HF call 0x20007408 incoming
         codec negotiation: 1
         codec auto selected: id 1
         HF SCO connected 0x20005248
         HF SCO info:
           SCO handle 0x0008
           SCO air mode 2
           SCO link type 2
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         HF call 0x20007408 ring
         HF call 0x20007408 CLIP 123456 0
         uart:~$ hfp hf accept 0
         HF call 0x20007408 accepted

Call termination
****************

After the call (outgoing or incoming) is accepted, it can be terminated from either the AG
(Audio Gateway) or HF (Hands-Free) side.

Terminate a call process from the AG:

.. tabs::

   .. group-tab:: Call termination on AG side

      .. code-block:: console

         uart:~$ hfp ag terminate 0
         AG call 0x20007690 terminate
         AG SCO disconnected 0x20005248 (reason 22)

   .. group-tab:: Call termination on HF side

      .. code-block:: console

         HF call 0x20007408 terminated
         HF SCO disconnected 0x20005248 (reason 22)

Terminate a call process from the HF:

.. tabs::

   .. group-tab:: Call termination on AG side

      .. code-block:: console

         AG call 0x20007690 terminate
         AG SCO disconnected 0x20005248 (reason 22)

   .. group-tab:: Call termination on HF side

      .. code-block:: console

         uart:~$ hfp hf terminate 0
         HF call 0x20007408 terminated
         HF SCO disconnected 0x20005248 (reason 22)

Terminate a call process from the remote:

.. tabs::

   .. group-tab:: Call termination on AG side

      .. code-block:: console

         uart:~$ hfp ag remote_terminate 0
         AG call 0x20007690 terminate
         AG SCO disconnected 0x20005248 (reason 22)

   .. group-tab:: Call termination on HF side

      .. code-block:: console

         HF call 0x20007408 terminated
         HF SCO disconnected 0x20005248 (reason 22)
