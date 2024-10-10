Bluetooth: Call Control Profile
###############################

Call Control Server
*******************
The Call Control Server is a role that typically resides on devices that can make calls,
including calls from apps such as Skype, e.g. (smart)phones and PCs,
which are typically GAP Central devices.

Using the Call Control Server
=============================
The Server can be controlled locally, or by a remote device (when in a call). For
example a remote device may initiate a call to the server,
or the Server may initiate a call to remote device, without a client.

.. code-block:: console

   ccp_server --help
   ccp_server - Bluetooth CCP server shell commands
   Subcommands:
     init  : Initialize CCP server

Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   uart:~$ ccp_server init
   Registered GTBS bearer
   Registered bearer[1]
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

Call Control Client
*******************
The Call Control Client is a role that typically resides on resource contrained devices such as
earbuds or headsets.

Using the Call Control Client
=============================
The Client can control a remote CCP server device.
For example a remote device may have an incoming call that can be accepted by the Client.

.. code-block:: console

   uart:~$ ccp_client --help
   ccp_client - Bluetooth CCP client shell commands
   Subcommands:
     discover  : Discover GTBS and TBS on remote device

Example Usage when connected
============================

.. code-block:: console

   uart:~$ ccp_client discover
   Discovery completed with GTBS and 1 TBS bearers
