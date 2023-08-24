Bluetooth: Gaming Audio Profile Shell
#####################################

This document describes how to run the Gaming Audio Profile shell functionality.
Unlike most other low-layer profiles, GMAP is a profile that exists and has a service (GMAS) on all
devices. Thus both the initiator and acceptor (or central and peripheral) should do a discovery of
the remote device's GMAS to see what GMAP roles and features they support.

Using the GMAP Shell
********************

When the Bluetooth stack has been initialized (:code:`bt init`), the GMAS can be registered by
by calling :code:`gmap init`. It is also strongly suggested to enable BAP via :code:`bap init`.

.. code-block:: console

   gmap --help
   gmap - Bluetooth GMAP shell commands
   Subcommands:
     init      :[none]
     set_role  :[ugt | ugg | bgr | bgs]
     discover  :[none]
     ac_1      :<sink preset>
     ac_2      :<source preset>
     ac_3      :<sink preset> <source preset>
     ac_4      :<sink preset>
     ac_5      :<sink preset> <source preset>
     ac_6_i    :<sink preset>
     ac_6_ii   :<sink preset>
     ac_7_ii   :<sink preset> <source preset>
     ac_8_i    :<sink preset> <source preset>
     ac_8_ii   :<sink preset> <source preset>
     ac_11_i   :<sink preset> <source preset>
     ac_11_ii  :<sink preset> <source preset>
     ac_12     :<preset>
     ac_13     :<preset>
     ac_14     :<preset>

The :code:`set_role` command can be used to change the role at runtime, assuming that the device
supports the role (the GMAP roles depend on some BAP configurations).

Example Central with GMAP UGT role
**********************************

Connect and establish Gaming Audio streams using Audio Configuration (AC) 3
(some logging has been omitted for clarity):

.. code-block:: console

   uart:~$ bt init
   uart:~$ bap init
   uart:~$ gmap init
   uart:~$ bt connect <address>
   uart:~$ gatt exchange-mtu
   uart:~$ bap discover
   Discover complete: err 0
   uart:~$ cap_initiator discover
   discovery completed with CSIS
   uart:~$ gmap discover
   gmap discovered for conn 0x2001c7d8:
        role 0x0f
        ugg_feat 0x07
        ugt_feat 0x6f
        bgs_feat 0x01
        bgr_feat 0x03
   uart:~$ gmap ac_3 32_2_gr 32_2_gs
   Starting 2 streams for AC_3
   stream 0x20020060 config operation rsp_code 0 reason 0
   stream 0x200204d0 config operation rsp_code 0 reason 0
   stream 0x200204d0 qos operation rsp_code 0 reason 0
   stream 0x20020060 qos operation rsp_code 0 reason 0
   Stream 0x20020060 enabled
   stream 0x200204d0 enable operation rsp_code 0 reason 0
   Stream 0x200204d0 enabled
   stream 0x20020060 enable operation rsp_code 0 reason 0
   Stream 0x20020060 started
   stream 0x200204d0 start operation rsp_code 0 reason 0
   Stream 0x200204d0 started
   Unicast start completed
   uart:~$ bap start_sine
   Started transmitting on default_stream 0x20020060
   [0]: stream 0x20020060 : TX LC3: 80 (seq_num 24800)
