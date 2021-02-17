.. _bluetooth_shell_bap:

Bluetooth: Basic Audio Profile
##############################

This document describes how to run basic audio profile functionality which
includes:

  - Capabilities and Endpoint discovery
  - Audio Stream Endpoint procedures
  - Managing, linking and unlinking audio stream endpoints

Commands
********

.. code-block:: console

   bap --help
   ase - ASE related commands
   Subcommands:
     init      :
     discover  :<type: sink, source>
     preset    :[preset]
     config    :<ase> <direction: sink, source> [codec] [preset]
     qos       :[preset] [interval] [framing] [latency] [pd] [sdu] [phy] [rtn]
     enable    :
     start     :
     disable   :
     stop      :
     release   :
     list      :
     select    :<ase>
     link      :<ase1> <ase2>
     unlink    :<ase1> <ase2>

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
   "select","none","any","none"
   "link","config","any","none"
   "unlink","link","any","none"

Example Central
***************

Connect and estabilish a stream:

.. code-block:: console

   uart:~$ bt init
   uart:~$ bap init
   uart:~$ bt connect <address>
   uart:~$ gatt exchange-mtu
   uart:~$ bap discover 0x01
   uart:~$ bap config 0x01 0x01
   uart:~$ bap qos
   uart:~$ bap enable

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

Init
****

This command register local PAC records which are necessary to be able to
configure stream and properly manage capabilities in use.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "none","any","none"

.. code-block:: console

   uart:~$ bap init

Discover PAC(s) and ASE(s)
**************************

Once connected this commands discover PAC records and ASE characteristics
representing remote endpoints.

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
   uart:~$ bap discover <dir>
   uart:~$ bap discover 0x01
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

This command can be used to either print the default preset configuration or set
a different one, it is worth noting that it doesn't change any stream previously
configured.

.. code-block:: console

   uart:~$ bap preset [preset]
   uart:~$ bap preset
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

This command attempts to configure a stream for the given direction using a
preset codec configuration which can either be passed directly or in case it is
omitted the default preset is used.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "discover","idle/codec-configured/qos-configured","codec-configured"

.. code-block:: console

   uart:~$ bap config <ase> <direction: sink, source> [codec] [preset]
   uart:~$ bap config 0x01 0x01
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
   ASE Codec Config chan 0x8179e60
   Default ase: 1
   ASE config: preset 16_2_1

Configure QoS
*************

This command attempts to configure the stream QoS using the preset
configuration, each individual QoS parameter can be set with use optional
parameters.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "config","qos-configured/codec-configured","qos-configured"

.. code-block:: console

   uart:~$ bap qos [preset] [interval] [framing] [latency] [pd] [sdu] [phy] [rtn]
   uart:~$ bap qos
   ASE config: preset 16_2_1

Enable
******

This command attempts to enable the stream previously configured, if the
remote peer accepts then the ISO connection proceedure is also initiated.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "qos","qos-configured","enabling"

.. code-block:: console

   uart:~$ bap enable

Start [sink only]
*****************

This command is only necessary when acting as a sink as it indicates to the
source the stack is ready to start receiving data.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "enable","enabling","streaming"

.. code-block:: console

   uart:~$ bap start

Disable
*******

This command attempts to disable the stream previously enabled, if the
remote peer accepts then the ISO disconnection proceedure is also initiated.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "enable","enabling/streaming","disabling"

.. code-block:: console

   uart:~$ bap disable

Stop [sink only]
****************

This command is only necessary when acting as a sink as it indicates to the
source the stack is ready to stop receiving data.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "disable","disabling","qos-configure/idle"

.. code-block:: console

   uart:~$ bap stop

Release
*******

This command releases the current stream and its configuration.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "config","any","releasing/codec-configure/idle"

.. code-block:: console

   uart:~$ bap release

List
****

This command list the available streams.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "none","any","none"

.. code-block:: console

   uart:~$ bap list
   *0: ase 0x01 dir 0x01 state 0x01 linked no

Select
******

This command set a stream as default.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "none","any","none"

.. code-block:: console

   uart:~$ bap select <ase>
   uart:~$ bap select 0x01
   Default ase: 1

Link
****

This command link streams so any command send to either of them is send to the
other as well, causing their state machine to be synchronized.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "config","any","none"

.. code-block:: console

   uart:~$ bap link <ase1> <ase2>
   uart:~$ bap link 0x01 0x02
   ases 1:2 linked

Unlink
******

This command unlink streams which were previously linked.

.. csv-table:: State Machine Transitions
   :header: "Depends", "Allowed States", "Next States"
   :widths: auto

   "link","any","none"

.. code-block:: console

   uart:~$ bap unlink <ase1> <ase2>
   uart:~$ bap unlink 0x01 0x02
   ases 1:2 unlinked
