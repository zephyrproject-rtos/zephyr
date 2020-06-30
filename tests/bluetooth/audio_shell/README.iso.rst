Bluetooth: Isochronous Channels
################################

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

1. [Master] Bind ISO connection to ACL:

Requires to be connected:

.. code-block:: console

   uart:~$ iso bind

2. [Master] Connect ISO channel:

.. code-block:: console

   uart:~$ iso connect

3. Send data:

.. code-block:: console

   uart:~$ iso send

4. Disconnect ISO channel:

.. code-block:: console

   uart:~$ iso disconnect
