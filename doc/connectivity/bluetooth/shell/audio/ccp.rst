Bluetooth: Call Control Profile Shell
#####################################

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

   ccp_call_control_server --help
   ccp_call_control_server - Bluetooth CCP Call Control Server shell commands
   Subcommands:
     init  : Initialize CCP Call Control Server

Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   uart:~$ ccp_call_control_server init
   Registered GTBS bearer
   Registered bearer[1]
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public
