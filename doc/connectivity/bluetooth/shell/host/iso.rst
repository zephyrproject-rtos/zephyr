Bluetooth: Isochronous Channels Shell
#####################################

Commands
********

.. code-block:: console

   iso --help
   iso - Bluetooth ISO shell commands
   Subcommands:
      cig_create  :[dir=tx,rx,txrx] [interval] [packing] [framing] [latency] [sdu]
                  [phy] [rtn]
      cig_term    :Terminate the CIG
      connect     :Connect ISO Channel
      listen      :<dir=tx,rx,txrx> [security level]
      send        :Send to ISO Channel [count]
      disconnect  :Disconnect ISO Channel
      create-big  :Create a BIG as a broadcaster [enc <broadcast code>]
      broadcast   :Broadcast on ISO channels
      sync-big    :Synchronize to a BIG as a receiver <BIS bitfield> [mse] [timeout]
                  [enc <broadcast code>]
      term-big    :Terminate a BIG


Unicast examples
****************
1. [Central] Create CIG:

Requires to be connected:

.. code-block:: console

   uart:~$ iso cig_create
   CIG created

2. [Peripheral] Listen to ISO connections

.. code-block:: console

   uart:~$ iso listen txrx

3. [Central] Connect ISO channel:

.. code-block:: console

   uart:~$ iso connect
   ISO Connect pending...
   ISO Channel 0x20000f88 connected

4. Send data:

.. code-block:: console

   uart:~$ iso send
   send: 40 bytes of data
   ISO sending...


5. Disconnect ISO channel:

.. code-block:: console

   uart:~$ iso disconnect
   ISO Disconnect pending...
   ISO Channel 0x20000f88 disconnected with reason 0x16


Broadcast examples
******************

Setting up broadcaster
======================

.. code-block:: console

   uart:~$ bt init
   Bluetooth initialized
   uart:~$ bt adv-create nconn-nscan ext-adv
   Created adv id: 0, adv: 0x200025d0
   uart:~$ bt per-adv-param
   uart:~$ iso create-big
   BIG created
   ISO Channel 0x200008c0 connected
   uart:~$
   uart:~$ bt adv-start
   Advertiser[0] 0x200025d0 set started
   uart:~$
   uart:~$ bt per-adv on
   Periodic advertising started
   uart:~$
   uart:~$ iso broadcast
   send: 247 bytes of data with PSN 4350
   ISO broadcasting...

If encrypted broadcast is required, then a broadcast code can be provided as

.. code-block:: console

   uart:~$ iso create-big enc 00112233445566778899aabbccddffff
   BIG created

Setting up sync receiver
========================

.. code-block:: console

   uart:~$ bt init
   Bluetooth initialized
   uart:~$ bt scan on
   [DEVICE]: 42:0F:7A:40:AE:21 (random), AD evt type 5, RSSI -27  C:0 S:0 D:0 SR:0 E:1 Prim: LE 1M, Secn: LE 2M, Interval: 0x0780 (2400000 us), SID: 0x0
   uart:~$ bt per-adv-sync-create 42:0F:7A:40:AE:21 (random) 0
   Per adv sync pending
   PER_ADV_SYNC[0]: [DEVICE]: 42:0F:7A:40:AE:21 (random) synced, Interval 0x0780 (2400000 us), PHY LE 2M, SD 0x0000, PAST peer not present
   PER_ADV_SYNC[0]: [DEVICE]: 42:0F:7A:40:AE:21 (random), tx_power 127, RSSI -28, CTE 0, data length 0
   BIG_INFO PER_ADV_SYNC[0]: [DEVICE]: 42:0F:7A:40:AE:21 (random), sid 0x00, num_bis 1, nse 0x04, interval 0x0008 (10000 us), bn 0x01, pto 0x01, irc 0x03, max_pdu 0x00f7, sdu_interval 0x2710, max_sdu 0x00f7, phy LE 2M, framing 0x00, not encrypted
   uart:~$ iso sync-big 1
   BIG syncing
   ISO Channel 0x200008c0 connected


If encrypted broadcast is required, then a broadcast code can be provided as

.. code-block:: console

   uart:~$ iso sync-big 1 enc 00112233445566778899aabbccddffff
   BIG syncing
   ISO Channel 0x200008c0 connected
