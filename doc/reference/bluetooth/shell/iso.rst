Bluetooth: Isochronous Channels
###############################

Commands
********

.. code-block:: console

   iso --help
   iso - ISO related commands
   Subcommands:
     bind        :Bind ISO Channel
     connect     :Connect ISO Channel
     listen      :Listen to ISO Channel
     send        :Send to ISO Channel
     disconnect  :Disconnect ISO Channel

Bind to ACL connection [central]
********************************

Requires to be connected (e.g. using :code:`bt connect`):

.. code-block:: console

   uart:~$ iso bind

Connect [central]
*****************

This command attempt to connect the ISO channel that has been previously
bound to an ACL using CIS connection procedure:

.. code-block:: console

   uart:~$ iso connect

Listen [peripheral]
*******************

This command register an ISO channel to listen to incoming connections:

.. code-block:: console

   uart:~$ iso listen

Send
****

This command sends data over a previously connected ISO channel:

.. code-block:: console

   uart:~$ iso send

Disconnect
**********

This command disconnects a previously connected ISO channel:

.. code-block:: console

   uart:~$ iso disconnect
