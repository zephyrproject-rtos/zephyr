Bluetooth: Classic: SPP Shell
#############################

This document describes how to run the Bluetooth Classic SPP functionality.
The :code:`spp` command exposes the Bluetooth Classic SPP Shell commands.

Commands
********

The :code:`spp` commands:

.. code-block:: console

   uart:~$ spp
   spp - Bluetooth SPP sh commands
   Subcommands:
     register_with_channel  : <channel>
     register_with_uuid     : <bt-uuid16|bt-uuid32|bt-uuid128>
     connect_by_channel     : <channel>
     connect_by_uuid        : <bt-uuid16|bt-uuid32|bt-uuid128>
     send                   : [length of packet(s)]
     disconnect             : [none]

Server Register
***************

.. tabs::

   .. group-tab:: Register with channel

      .. code-block:: console

         uart:~$ spp register_with_channel 9
         SPP: server registered (channel=9)

   .. group-tab:: Register with UUID

      .. code-block:: console

         uart:~$ spp register_with_uuid 00001101-0000-1000-8000-00805F9B34FB
         SPP: server registered (uuid=00001101-0000-1000-8000-00805f9b34fb, channel=9)

Connect
*******

The ACL connection should be established before creating the SPP connection.

.. tabs::

   .. group-tab:: Server (incoming)

      .. code-block:: console

         uart:~$ spp register_with_channel 9
         SPP: server registered (channel=9)
         SPP: accepted incoming connection (conn=0x20004dc8)
         SPP: connected (ep=0x20000d20, channel=9)

   .. group-tab:: Client by channel

      .. code-block:: console

         uart:~$ spp connect_by_channel 9
         SPP: connect started (channel=9)
         SPP: connected (ep=0x20000d20, channel=9)

   .. group-tab:: Client by UUID

      .. code-block:: console

         uart:~$ spp connect_by_uuid 00001101-0000-1000-8000-00805F9B34FB
         SPP: connect started (uuid=00001101-0000-1000-8000-00805f9b34fb)
         SPP: connected (ep=0x20000d20, channel=9)

Send Data
*********

.. code-block:: console

   uart:~$ spp send 5
   SPP: tx data (len=5)

Disconnect
**********

.. code-block:: console

   uart:~$ spp disconnect
   SPP: disconnecting...
   SPP: disconnected (ep=0x20000d20)
