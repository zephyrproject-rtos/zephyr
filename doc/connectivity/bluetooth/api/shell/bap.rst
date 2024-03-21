.. _bluetooth_shell_audio:

Bluetooth: Basic Audio Profile
##############################

This document describes how to run Basic Audio Profile functionality which
includes:

  - Capabilities and Endpoint discovery
  - Audio Stream Endpoint procedures

Commands
********

.. code-block:: console

   bap --help
   Subcommands:
      init
      select_broadcast       : <stream>
      create_broadcast       : [preset <preset_name>] [enc <broadcast_code>]
      start_broadcast        :
      stop_broadcast         :
      delete_broadcast       :
      create_broadcast_sink  : 0x<broadcast_id>
      sync_broadcast         : 0x<bis_index> [[[0x<bis_index>] 0x<bis_index>] ...]
                              [bcode <broadcast code> || bcode_str <broadcast code
                              as string>]
      stop_broadcast_sink    : Stops broadcast sink
      term_broadcast_sink    :
      discover               : [dir: sink, source]
      config                 : <direction: sink, source> <index> [loc <loc_bits>]
                              [preset <preset_name>]
      stream_qos             : interval [framing] [latency] [pd] [sdu] [phy] [rtn]
      qos                    : Send QoS configure for Unicast Group
      enable                 : [context]
      stop
      list
      print_ase_info         : Print ASE info for default connection
      metadata               : [context]
      start
      disable
      release
      select_unicast         : <stream>
      preset                 : <sink, source, broadcast> [preset]
                              [config
                                    [freq <frequency>]
                                    [dur <duration>]
                                    [chan_alloc <location>]
                                    [frame_len <frame length>]
                                    [frame_blks <frame blocks>]]
                              [meta
                                    [pref_ctx <context>]
                                    [stream_ctx <context>]
                                    [program_info <program info>]
                                    [stream_lang <ISO 639-3 lang>]
                                    [ccid_list <ccids>]
                                    [parental_rating <rating>]
                                    [program_info_uri <URI>]
                                    [audio_active_state <state>]
                                    [bcast_flag]
                                    [extended <meta>]
                                    [vendor <meta>]]
      send                   : Send to Audio Stream [data]
      start_sine             : Start sending a LC3 encoded sine wave [all]
      stop_sine              : Stop sending a LC3 encoded sine wave [all]
      recv_stats             : Sets or gets the receive statistics reporting interval
                              in # of packets
      set_location           : <direction: sink, source> <location bitmask>
      set_context            : <direction: sink, source><context bitmask> <type:
                              supported, available>


.. csv-table:: State Machine Transitions
   :header: "Command", "Depends", "Allowed States", "Next States"
   :widths: auto

   "init","none","any","none"
   "discover","init","any","any"
   "config","discover","idle/codec-configured/qos-configured","codec-configured"
   "qos","config","codec-configured/qos-configured","qos-configured"
   "enable","qos","qos-configured","enabling"
   "[start]","enable","enabling","streaming"
   "disable","enable", "enabling/streaming","disabling"
   "[stop]","disable","disabling","qos-configure/idle"
   "release","config","any","releasing/codec-configure/idle"
   "list","none","any","none"
   "select_unicast","none","any","none"
   "connect","discover","idle/codec-configured/qos-configured","codec-configured"
   "send","enable","streaming","none"

Example Central
***************

Connect and establish a stream:

.. code-block:: console

   uart:~$ bt init
   uart:~$ bap init
   uart:~$ bt connect <address>
   uart:~$ gatt exchange-mtu
   uart:~$ bap discover sink
   uart:~$ bap config sink 0
   uart:~$ bap qos
   uart:~$ bap enable

Or using connect command:

.. code-block:: console

   uart:~$ bt init
   uart:~$ bap init
   uart:~$ bt connect <address>
   uart:~$ gatt exchange-mtu
   uart:~$ bap discover sink
   uart:~$ bap connect sink 0

Disconnect and release:

.. code-block:: console

   uart:~$ bap disable
   uart:~$ bap release

Example Peripheral
******************

Listen:

.. code-block:: console

   uart:~$ bt init
   uart:~$ bap init
   uart:~$ bt advertise on

Server initiated disable and release:

.. code-block:: console

   uart:~$ bap disable
   uart:~$ bap release

Example Broadcast Source
************************

Create and establish a broadcast source stream:

.. code-block:: console

   uart:~$ bap init
   uart:~$ bap create_broadcast
   uart:~$ bap start_broadcast

Stop and release a broadcast source stream:

.. code-block:: console

   uart:~$ bap stop_broadcast
   uart:~$ bap delete_broadcast


Example Broadcast Sink
**********************

Scan for and establish a broadcast sink stream.
The command :code:`bap create_broadcast_sink 0xEF6716` will either use existing periodic advertising
sync (if exist) or start scanning and sync to the periodic advertising before syncing to the BIG.

.. code-block:: console

   uart:~$ bap init
   uart:~$ bap broadcast_scan on
   Found broadcaster with ID 0xEF6716 and addr 3D:A5:F9:35:0B:19 (random) and sid 0x00
   uart:~$ bap create_broadcast_sink 0xEF6716
   Attempting to PA sync to the broadcaster
   PA synced to broadcast with broadcast ID 0xEF6716
   Attempting to sync to the BIG
   Received BASE from sink 0x20031fac:
   Presentation delay: 40000
   Subgroup count: 2
   Subgroup[0]:
   codec cfg id 0x06 cid 0x0000 vid 0x0000
   data_count 4
   data #0: type 0x01 len 1
   00000000: 03                                               |.                |
   data #1: type 0x02 len 1
   00000000: 01                                               |.                |
   data #2: type 0x03 len 4
   00000000: 01 00 00 00                                      |....             |
   data #3: type 0x04 len 2
   00000000: 28 00                                            |(.               |
   meta_count 4
   meta #0: type 0x02 len 2
   00000000: 01 00                                            |..               |
      BIS[0] index 0x01
   Subgroup[1]:
   codec cfg id 0x06 cid 0x0000 vid 0x0000
   data_count 4
   data #0: type 0x01 len 1
   00000000: 03                                               |.                |
   data #1: type 0x02 len 1
   00000000: 01                                               |.                |
   data #2: type 0x03 len 4
   00000000: 01 00 00 00                                      |....             |
   data #3: type 0x04 len 2
   00000000: 28 00                                            |(.               |
   meta_count 4
   meta #0: type 0x02 len 2
   00000000: 01 00                                            |..               |
      BIS[1] index 0x01
   [0]: 0x01
   [1]: 0x01
   Possible indexes: 0x01 0x01
   Sink 0x20019110 is ready to sync without encryption
   uart:~$ bap sync_broadcast 0x01

Syncing to encrypted broadcast
------------------------------

If the broadcast is encrypted, the broadcast code can be entered with the :code:`bap sync_broadcast`
command as such:

.. code-block:: console

   Sink 0x20019110 is ready to sync with encryption
   uart:~$ bap sync_broadcast 0x01 bcode 0102030405060708090a0b0c0d0e0f

The broadcast code can be 1-16 values, either as a string or a hexadecimal value.

.. code-block:: console

   Sink 0x20019110 is ready to sync with encryption
   uart:~$ bap sync_broadcast 0x01 bcode_str thisismycode

Stop and release a broadcast sink stream:

.. code-block:: console

   uart:~$ bap stop_broadcast_sink
   uart:~$ bap term_broadcast_sink

Init
****

The :code:`init` command register local PAC records which are necessary to be
able to configure stream and properly manage capabilities in use.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "none","any","none"

.. code-block:: console

   uart:~$ bap init

Discover PAC(s) and ASE(s)
**************************

Once connected the :code:`discover` command discover PAC records and ASE
characteristics representing remote endpoints.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "init","any","any"

.. note::

   Use command :code:`gatt exchange-mtu` to make sure the MTU is configured
   properly.

.. code-block:: console

   uart:~$ gatt exchange-mtu
   Exchange pending
   Exchange successful
   uart:~$ bap discover [type: sink, source]
   uart:~$ bap discover sink
   cap 0x8175940 type 0x01
   codec 0x06 cid 0x0000 vid 0x0000 count 4
   data #0: type 0x01 len 1
   00000000: 3f                                             |?                |
   data #1: type 0x02 len 1
   00000000: 03                                             |.                |
   data #2: type 0x03 len 1
   00000000: 03                                             |.                |
   data #3: type 0x04 len 4
   00000000: 1e 00 f0 00                                    |....             |
   meta #0: type 0x01 len 2
   00000000: 06 00                                          |..               |
   meta #1: type 0x02 len 2
   00000000: ff 03                                          |..               |
   ep 0x81754e0
   ep 0x81755d4
   Discover complete: err 0

Select preset
*************

The :code:`preset` command can be used to either print the default preset
configuration or set a different one, it is worth noting that it doesn't change
any stream previously configured.

.. code-block:: console

   uart:~$ bap preset
   preset - <sink, source, broadcast> [preset]
            [config
                  [freq <frequency>]
                  [dur <duration>]
                  [chan_alloc <location>]
                  [frame_len <frame length>]
                  [frame_blks <frame blocks>]]
            [meta
                  [pref_ctx <context>]
                  [stream_ctx <context>]
                  [program_info <program info>]
                  [stream_lang <ISO 639-3 lang>]
                  [ccid_list <ccids>]
                  [parental_rating <rating>]
                  [program_info_uri <URI>]
                  [audio_active_state <state>]
                  [bcast_flag]
                  [extended <meta>]
                  [vendor <meta>]]
   uart:~$ bap preset sink
   16_2_1
   codec 0x06 cid 0x0000 vid 0x0000 count 4
   data #0: type 0x01 len 1
   data #1: type 0x02 len 1
   data #2: type 0x03 len 4
   00000000: 01 00 00                                         |...              |
   data #3: type 0x04 len 2
   00000000: 28                                               |(                |
   meta #0: type 0x02 len 2
   00000000: 06                                               |.                |
   QoS: interval 10000 framing 0x00 phy 0x02 sdu 40 rtn 2 latency 10 pd 40000

   uart:~$ bap preset sink 32_2_1
   32_2_1
   codec 0x06 cid 0x0000 vid 0x0000 count 4
   data #0: type 0x01 len 1
   data #1: type 0x02 len 1
   data #2: type 0x03 len 4
   00000000: 01 00 00                                         |...              |
   data #3: type 0x04 len 2
   00000000: 50                                               |P                |
   meta #0: type 0x02 len 2
   00000000: 06                                               |.                |
   QoS: interval 10000 framing 0x00 phy 0x02 sdu 80 rtn 2 latency 10 pd 40000

Configure preset
****************

The :code:`bap preset` command can also be used to configure the preset used for the subsequent
commands. It is possible to add or set (or reset) any value. To reset the preset, the command can \
simply be run without the :code:`config` or :code:`meta` parameter. The parameters are using the
assigned numbers values.

.. code-block:: console

   uart:~$ bap preset sink 32_2_1
   32_2_1
   codec cfg id 0x06 cid 0x0000 vid 0x0000 count 16
   data #0: type 0x01 value_len 1
   00000000: 06                                               |.                |
   data #1: type 0x02 value_len 1
   00000000: 01                                               |.                |
   data #2: type 0x03 value_len 4
   00000000: 03 00 00 00                                      |....             |
   data #3: type 0x04 value_len 2
   00000000: 50 00                                            |P.               |
   meta #0: type 0x02 value_len 2
   00000000: 08 00                                            |..               |
   QoS: interval 10000 framing 0x00 phy 0x02 sdu 80 rtn 2 latency 10 pd 40000

   uart:~$ bap preset sink 32_2_1 config freq 10
   32_2_1
   codec cfg id 0x06 cid 0x0000 vid 0x0000 count 16
   data #0: type 0x01 value_len 1
   00000000: 0a                                               |.                |
   data #1: type 0x02 value_len 1
   00000000: 01                                               |.                |
   data #2: type 0x03 value_len 4
   00000000: 03 00 00 00                                      |....             |
   data #3: type 0x04 value_len 2
   00000000: 50 00                                            |P.               |
   meta #0: type 0x02 value_len 2
   00000000: 08 00                                            |..               |
   QoS: interval 10000 framing 0x00 phy 0x02 sdu 80 rtn 2 latency 10 pd 40000

   uart:~$ bap preset sink 32_2_1 config freq 10 meta stream_lang "eng" stream_ctx 4
   32_2_1
   codec cfg id 0x06 cid 0x0000 vid 0x0000 count 16
   data #0: type 0x01 value_len 1
   00000000: 0a                                               |.                |
   data #1: type 0x02 value_len 1
   00000000: 01                                               |.                |
   data #2: type 0x03 value_len 4
   00000000: 03 00 00 00                                      |....             |
   data #3: type 0x04 value_len 2
   00000000: 50 00                                            |P.               |
   meta #0: type 0x02 value_len 2
   00000000: 04 00                                            |..               |
   meta #1: type 0x04 value_len 3
   00000000: 65 6e 67                                         |eng              |
   QoS: interval 10000 framing 0x00 phy 0x02 sdu 80 rtn 2 latency 10 pd 40000

Configure Codec
***************

The :code:`config` command attempts to configure a stream for the given
direction using a preset codec configuration which can either be passed directly
or in case it is omitted the default preset is used.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "discover","idle/codec-configured/qos-configured","codec-configured"

.. code-block:: console

   uart:~$ bap config <direction: sink, source> <index> [loc <loc_bits>] [preset <preset_name>]
   uart:~$ bap config sink 0
   ASE Codec Config: conn 0x8173800 ep 0x81754e0 cap 0x816a360
   codec 0x06 cid 0x0000 vid 0x0000 count 3
   data #0: type 0x01 len 1
   00000000: 02                                             |.                |
   data #1: type 0x02 len 1
   00000000: 01                                             |.                |
   data #2: type 0x04 len 2
   00000000: 28 00                                          |(.               |
   meta #0: type 0x02 len 2
   00000000: 02 00                                          |..               |
   ASE Codec Config stream 0x8179e60
   Default ase: 1
   ASE config: preset 16_2_1

Configure Stream QoS
********************

The :code:`stream_qos` Sets a new stream QoS.

.. code-block:: console

   uart:~$ bap stream_qos <interval> [framing] [latency] [pd] [sdu] [phy] [rtn]
   uart:~$ bap stream_qos 10

Configure QoS
*************

The :code:`qos` command attempts to configure the stream QoS using the preset
configuration, each individual QoS parameter can be set with use optional
parameters.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "config","qos-configured/codec-configured","qos-configured"

.. code-block:: console

   uart:~$ bap qos

Enable
******

The :code:`enable` command attempts to enable the stream previously configured,
if the remote peer accepts then the ISO connection procedure is also initiated.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "qos","qos-configured","enabling"

.. code-block:: console

   uart:~$ bap enable [context]
   uart:~$ bap enable Media

Start
*****

The :code:`start` command is only necessary when acting as a sink as it
indicates to the source the stack is ready to start receiving data.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "enable","enabling","streaming"

.. code-block:: console

   uart:~$ bap start

Disable
*******

The :code:`disable` command attempts to disable the stream previously enabled,
if the remote peer accepts then the ISO disconnection procedure is also
initiated.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "enable","enabling/streaming","disabling"

.. code-block:: console

   uart:~$ bap disable

Stop
****

The :code:`stop` command is only necessary when acting as a sink as it indicates
to the source the stack is ready to stop receiving data.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "disable","disabling","qos-configure/idle"

.. code-block:: console

   uart:~$ bap stop

Release
*******

The :code:`release` command releases the current stream and its configuration.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "config","any","releasing/codec-configure/idle"

.. code-block:: console

   uart:~$ bap release

List
****

The :code:`list` command list the available streams.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "none","any","none"

.. code-block:: console

   uart:~$ bap list
   *0: ase 0x01 dir 0x01 state 0x01

Select Unicast
**************

The :code:`select_unicast` command set a unicast stream as default.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "none","any","none"

.. code-block:: console

   uart:~$ bap select <ase>
   uart:~$ bap select 0x01
   Default stream: 1

To select a broadcast stream:

.. code-block:: console

   uart:~$ bap select 0x01 broadcast
   Default stream: 1 (broadcast)

Send
****

The :code:`send` command sends data over BAP Stream.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "enable","streaming","none"

.. code-block:: console

   uart:~$ bap send [count]
   uart:~$ bap send
   Audio sending...
