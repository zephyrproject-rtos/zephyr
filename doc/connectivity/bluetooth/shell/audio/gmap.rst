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

   uart:~$ gmap --help
   gmap - Bluetooth GMAP shell commands
   Subcommands:
     init      : [none]
     set_role  : [ugt | ugg | bgr | bgs]
     discover  : [none]
     ac_1      : Unicast audio configuration 1
     ac_2      : Unicast audio configuration 2
     ac_3      : Unicast audio configuration 3
     ac_4      : Unicast audio configuration 4
     ac_5      : Unicast audio configuration 5
     ac_6_i    : Unicast audio configuration 6(i)
     ac_6_ii   : Unicast audio configuration 6(ii)
     ac_7_ii   : Unicast audio configuration 7(ii)
     ac_8_i    : Unicast audio configuration 8(i)
     ac_8_ii   : Unicast audio configuration 8(ii)
     ac_11_i   : Unicast audio configuration 11(i)
     ac_11_ii  : Unicast audio configuration 11(ii)
     ac_12     : Broadcast audio configuration 12
     ac_13     : Broadcast audio configuration 13
     ac_14     : Broadcast audio configuration 14

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
   uart:~$ bap preset sink 32_2_gr
   uart:~$ bap preset source 32_2_gs
   uart:~$ gmap ac_3
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
