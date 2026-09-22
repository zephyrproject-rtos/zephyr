.. zephyr:code-sample:: ssh-server-client
   :name: SSH server and client

   Implements SSH server and client shell support.

Overview
********

The SSH sample application for Zephyr enables SSH server and client
functionality. The network shell can be used to listen incoming
SSH connections, or connecting to external host using SSH.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/ssh`.

Requirements
************

- :ref:`networking_with_host`

Building
********

Build ssh sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ssh
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Attach to the Zephyr UART shell using your favourite terminal emulator (replace ``/dev/pts/12``
with whatever Zephyr printed out above e.g ``uart connected to pseudotty: /dev/pts/12``):

.. code-block:: console

   python -m serial.tools.miniterm /dev/pts/12 --raw --eol CRLF

Generating Keys
***************

Then in network shell you can generate and save host keys on the first run,
in this example host key index is set to 0.

.. code-block:: console

   net ssh_key gen 0 rsa 2048
   net ssh_key save 0 priv id_rsa

On subsequent runs you can instead load the host key.

.. code-block:: console

   net ssh_key load 0 priv id_rsa


SSH Server
**********

Start SSH server (server instance 0, host key index 0)

.. code-block:: console

   net sshd start -i 0 -b 192.0.2.1 -k 0 -p password123

If you omit the ``-b`` option or bind to ``0.0.0.0``, then the server will
listen on all addresses. You can also use IPv6 address when binding to local
address.

In host computer shell:

.. code-block:: console

   ssh root@192.0.2.1

To exit press 'Enter' then '~' then '.' (i.e. enter tilde dot)


SSH Client
**********

Connect to your host computer from the Zephyr SSH client (client instance 0)
Replace <username> with the desired host computer user.

.. code-block:: console

   net ssh start username@192.0.2.2

Press ``Ctrl+d`` or type ``exit`` to exit and return to Zephyr.

You can also use IPv6 address or hostname when specifying the destination address.

.. code-block:: console

   net ssh start username@2001:db8::2
   net ssh start username@[2001:db8::2]:22
   net ssh start username@example.com
   net ssh start username@example.com:22


Client Public Key Auth
======================

Export the public key (host key index 0)

.. code-block:: console

   net ssh_key pub export 0

In host computer shell:

Convert the exported key to RFC4716 format and add to authorized_keys

.. code-block:: console

   ssh-keygen -i -f /dev/stdin -m pkcs8 <<< \
   '-----BEGIN PUBLIC KEY-----
   <your base-64 encoded host key>
   ------END PUBLIC KEY-----' \
   >> ~/.ssh/authorized_keys

Restart ssh client with extra host key argument, no password needed!
(client instance 0, host key index 0)

.. code-block:: console

   net ssh stop
   net ssh start -k 0 username@192.0.2.2


Server Public Key Auth
======================

Convert computer public key to PEM in host computer shell:

.. code-block:: console

   ssh-keygen -e -f ~/.ssh/id_rsa.pub -m PKCS8

In Zephyr shell import the public key into key index 1 (index 0 is reserved
for the server host key generated earlier)

.. code-block:: console

   net ssh_key pub import 1

Paste the output from above, followed by Ctrl+C.
Then save the public key (key index 1)

.. code-block:: console

   net ssh_key save 1 pub authorized_key_0

On subsequent runs you can instead load the public key

.. code-block:: console

   net ssh_key load 1 pub authorized_key_0

Restart ssh server with no password (disabled) and authorized key argument
(server instance 0, host key index 0, authorized key index 1)

.. code-block:: console

   net sshd stop -i 0
   net sshd start -i 0 -b 192.0.2.1:22 -k 0 -a 1

In host computer shell, connect to the Zephyr SSH server, no password needed.

.. code-block:: console

   ssh -i ~/.ssh/id_rsa root@192.0.2.1

To exit press 'Enter' then '~' then '.' (i.e. enter tilde dot).


Creating SSH Channels
*********************

In addition to the built-in ``net ssh`` and ``net sshd`` shell commands, the
sample registers a small ``sample`` command set that shows how an application
can create and remove SSH channels on top of an existing client connection.
Connection management itself is left to the SSH client shell commands described
above.

First establish a client connection, which authenticates and opens an
interactive session channel:

.. code-block:: console

   net ssh start username@192.0.2.2

Press ``Ctrl+d`` or type ``exit`` to return to the Zephyr shell while keeping
the connection open. Then create an additional channel on that connection:

.. code-block:: console

   sample open

The opened channels and the active server and client connections can be listed
with:

.. code-block:: console

   sample info

Finally, remove a channel by its number (as shown by ``sample info``):

.. code-block:: console

   sample close 1

The ``sample`` commands rely on the SSH transport callbacks: the sample
registers a server and a client transport callback to track the connections,
and uses :c:func:`ssh_transport_channel_open` and :c:func:`ssh_channel_close`
to create and remove channels.
