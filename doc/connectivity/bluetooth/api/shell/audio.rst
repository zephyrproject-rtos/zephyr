.. _bluetooth_shell_audio:

Bluetooth: Basic Audio Profile
##############################

This document describes how to run basic audio profile functionality which
includes:

  - Capabilities and Endpoint discovery
  - Audio Stream Endpoint procedures

Commands
********

.. code-block:: console

   audio --help
   Subcommands:
      init                 :
      discover             :[type: sink, source]
      preset               :[preset]
      config               :<direction: sink, source> <index> [codec] [preset]
      select_broadcast     :<stream>
      create_broadcast     :[codec] [preset]
      start_broadcast      :
      stop_broadcast       :
      delete_broadcast     :
      broadcast_scan       :<on, off>
      accept_broadcast     :0x<broadcast_id>
      sync_broadcast       :0x<bis_bitfield>
      stop_broadcast_sink  :Stops broadcast sink
      term_broadcast_sink  :
      qos                  :[preset] [interval] [framing] [latency] [pd] [sdu] [phy] [rtn]
      enable               :
      metadata             :[context]
      start                :
      disable              :
      stop                 :
      release              :
      list                 :
      select_unicast       :<stream>
      connect              :<direction: sink, source> <index>  [codec] [preset]
      send                 :Send to Audio Stream [data]

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
   uart:~$ audio init
   uart:~$ bt connect <address>
   uart:~$ gatt exchange-mtu
   uart:~$ audio discover sink
   uart:~$ audio config sink 0
   uart:~$ audio qos
   uart:~$ audio enable

Or using connect command:

.. code-block:: console

   uart:~$ bt init
   uart:~$ audio init
   uart:~$ bt connect <address>
   uart:~$ gatt exchange-mtu
   uart:~$ audio discover sink
   uart:~$ audio connect sink 0

Disconnect and release:

.. code-block:: console

   uart:~$ audio disable
   uart:~$ audio release

Example Peripheral
******************

Listen:

.. code-block:: console

   uart:~$ bt init
   uart:~$ audio init
   uart:~$ bt advertise on

Server initiated disable and release:

.. code-block:: console

   uart:~$ audio disable
   uart:~$ audio release

Example Broadcast Source
************************

Create and establish a broadcast source stream:

.. code-block:: console

   uart:~$ audio init
   uart:~$ audio create_broadcast
   uart:~$ audio start_broadcast

Stop and release a broadcast source stream:

.. code-block:: console

   uart:~$ audio stop_broadcast
   uart:~$ audio delete_broadcast


Example Broadcast Sink
************************

Scan for and establish a broadcast sink stream:

.. code-block:: console

   uart:~$ audio init
   uart:~$ audio broadcast_scan on
   Found broadcaster with ID 0xB91CD4
   uart:~$ audio accept_broadcast 0xB91CD4
   PA syncing to broadcaster
   Broadcast scan was terminated: 0
   PA synced to broadcaster with ID 0xB91CD4 as sink 0x2000d09c
   Sink 0x2000d09c is set as default
   Sink 0x2000d09c is ready to sync without encryption
   Received BASE from sink 0x2000d09c:
   Subgroup[0]:
   codec 0x06 cid 0x0000 vid 0x0000 count 4
   data #0: type 0x01 len 1
   data #1: type 0x02 len 1
   data #2: type 0x03 len 4
   00000000: 00 00 00                                         |...              |
   data #3: type 0x04 len 2
   00000000: 28                                               |(                |
   meta #0: type 0x02 len 2
   BIS[0] index 0x01
   [0]: 0x01
   Possible indexes: 0x01
   audio sync_broadcast 0x01

Stop and release a broadcast sink stream:

.. code-block:: console

   uart:~$ audio stop_broadcast_sink
   uart:~$ audio term_broadcast_sink

Init
****

The :code:`init` command register local PAC records which are necessary to be
able to configure stream and properly manage capabilities in use.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "none","any","none"

.. code-block:: console

   uart:~$ audio init

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
   uart:~$ audio discover [type: sink, source]
   uart:~$ audio discover sink
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

   uart:~$ audio preset [preset]
   uart:~$ audio preset
   16_2_1
   codec 0x06 cid 0x0000 vid 0x0000 count 3
   data #0: type 0x01 len 1
   00000000: 02                                             |.                |
   data #1: type 0x02 len 1
   00000000: 01                                             |.                |
   data #2: type 0x04 len 2
   00000000: 28 00                                          |(.               |
   meta #0: type 0x02 len 2
   00000000: 02 00                                          |..               |
   QoS: dir 0x02 interval 10000 framing 0x00 phy 0x02 sdu 40 rtn 2 latency 10 pd 40000

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

   uart:~$ audio config <direction: sink, source> <index> [codec] [preset]
   uart:~$ audio config sink 0
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

   uart:~$ audio qos [preset] [interval] [framing] [latency] [pd] [sdu] [phy] [rtn]
   uart:~$ audio qos
   ASE config: preset 16_2_1

Enable
******

The :code:`enable` command attempts to enable the stream previously configured,
if the remote peer accepts then the ISO connection procedure is also initiated.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "qos","qos-configured","enabling"

.. code-block:: console

   uart:~$ audio enable

Start [sink only]
*****************

The :code:`start` command is only necessary when acting as a sink as it
indicates to the source the stack is ready to start receiving data.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "enable","enabling","streaming"

.. code-block:: console

   uart:~$ audio start

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

   uart:~$ audio disable

Stop [sink only]
****************

The :code:`stop` command is only necessary when acting as a sink as it indicates
to the source the stack is ready to stop receiving data.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "disable","disabling","qos-configure/idle"

.. code-block:: console

   uart:~$ audio stop

Release
*******

The :code:`release` command releases the current stream and its configuration.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "config","any","releasing/codec-configure/idle"

.. code-block:: console

   uart:~$ audio release

List
****

The :code:`list` command list the available streams.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "none","any","none"

.. code-block:: console

   uart:~$ audio list
   *0: ase 0x01 dir 0x01 state 0x01

Select Unicast
**************

The :code:`select_unicast` command set a unicast stream as default.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "none","any","none"

.. code-block:: console

   uart:~$ audio select <ase>
   uart:~$ audio select 0x01
   Default stream: 1

To select a broadcast stream:

.. code-block:: console

   uart:~$ audio select 0x01 broadcast
   Default stream: 1 (broadcast)

Connect
*******

The :code:`connect` command combines config, qos and enable commands in one so
it can be used to quickly configure and enable a stream.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "discover","idle/codec-configured/qos-configured","streaming"

.. code-block:: console

   uart:~$ audio connect <direction: sink, source> <index> [codec] [preset]
   uart:~$ audio connect sink 0
   ASE Codec Config: conn 0x17ca40 ep 0x17f860 cap 0x19f6a0
   codec 0x06 cid 0x0000 vid 0x0000 count 3
   data #0: type 0x01 len 1
   00000000: 02                                               |.                |
   data #1: type 0x02 len 1
   00000000: 01                                               |.                |
   data #2: type 0x04 len 2
   00000000: 28 00                                            |(.               |
   meta #0: type 0x02 len 2
   00000000: 02 00                                            |..               |
   ASE Codec Config stream 0x1851c0
   Default ase: 1
   ASE config: preset 16_2_1
   ASE Codec Reconfig: stream 0x1851c0 cap 0x19f6a0
   codec 0x06 cid 0x0000 vid 0x0000 count 3
   data #0: type 0x01 len 1
   00000000: 02                                               |.                |
   data #1: type 0x02 len 1
   00000000: 01                                               |.                |
   data #2: type 0x04 len 2
   00000000: 28 00                                            |(.               |
   meta #0: type 0x02 len 2
   00000000: 02 00                                            |..               |
   QoS: stream 0x1851c0
   QoS: dir 0x02 interval 10000 framing 0x00 phy 0x02 sdu 40 rtn 2 latency 10 pd 40000
   Start: stream 0x1851c0

Send
****

The :code:`send` command sends data over Audio Stream.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "enable","streaming","none"

.. code-block:: console

   uart:~$ audio send [count]
   uart:~$ audio send
   Audio sending...
