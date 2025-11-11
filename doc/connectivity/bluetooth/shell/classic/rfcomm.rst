Bluetooth: Classic: RFCOMM Shell
################################

This document describes how to run the Bluetooth Classic RFCOMM functionality.
The :code:`rfcomm` command exposes the Bluetooth Classic RFCOMM Shell commands.

Commands
********

The :code:`rfcomm` commands:

.. code-block:: console

   uart:~$ rfcomm
   rfcomm - Bluetooth RFCOMM shell commands
   Subcommands:
     register    : [none]
     connect     : <channel>
     disconnect  : [none]
     send        : <number of packets>
     rpn         : Send RPN command with default settings

Connect
*******

The ACL connection should be established before creating the RFCOMM connection.

.. tabs::

   .. group-tab:: Non-initializing device

      .. code-block:: console

         uart:~$ rfcomm  register
         RFCOMM channel 5 registered
         Security changed: XX:XX:XX:XX:XX:XX level 2
         Incoming RFCOMM conn 0x20004dc8
         Dlc 0x20000d20 connected

   .. group-tab:: Initializing device

      .. code-block:: console

         uart:~$ rfcomm connect 5
         RFCOMM connection pending
         Security changed: XX:XX:XX:XX:XX:XX level 2
         Dlc 0x20000d20 connected

Send Data
*********

.. tabs::

   .. group-tab:: Non-initializing device

      .. code-block:: console

         Incoming data dlc 0x20000d20 len 30
         uart:~$ rfcomm send 1
         uart:~$

   .. group-tab:: Initializing device

      .. code-block:: console

         uart:~$ rfcomm send 1
         Incoming data dlc 0x20000d20 len 30

Disconnect
**********

Create disconnect request from non-initializing device:

.. tabs::

   .. group-tab:: Non-initializing device

      .. code-block:: console

         uart:~$ rfcomm disconnect
         Dlc 0x20000d20 disconnected
         uart:~$

   .. group-tab:: Initializing device

      .. code-block:: console

         Dlc 0x20000d20 disconnected
         uart:~$

Create disconnect request from initializing device:

.. tabs::

   .. group-tab:: Non-initializing device

      .. code-block:: console

         Dlc 0x20000d20 disconnected
         uart:~$

   .. group-tab:: Initializing device

      .. code-block:: console

         uart:~$ rfcomm disconnect
         Dlc 0x20000d20 disconnected
         uart:~$
