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
