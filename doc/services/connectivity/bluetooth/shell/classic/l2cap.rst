Bluetooth: Classic: L2CAP Shell
###############################

This document describes how to run the Bluetooth Classic L2CAP functionality.
The :code:`br l2cap` command exposes the Bluetooth Classic L2CAP Shell commands.

Commands
********

The :code:`br l2cap` commands:

.. code-block:: console

   uart:~$ br l2cap
   l2cap - [none]
   Subcommands:
     register    : <psm> <mode: none, ret, fc, eret, stream> [hold_credit]
                   [mode_optional] [extended_control]
     connect     : <psm> <mode: none, ret, fc, eret, stream> [hold_credit]
                   [mode_optional] [extended_control]
     disconnect  : [none]
     send        : [number of packets] [length of packet(s)]
     credits     : [none]
     echo        : L2CAP BR ECHO commands
     connless    : L2CAP connectionless commands

The :code:`br l2cap echo` commands:

.. code-block:: console

   uart:~$ br l2cap echo
   echo - L2CAP BR ECHO commands
   Subcommands:
     register    : [none]
     unregister  : [none]
     req         : <length of data>
     rsp         : <identifier> <length of data>

The :code:`br l2cap connless` commands:

.. code-block:: console

   uart:~$ br l2cap connless
   connless - L2CAP connectionless commands
   Subcommands:
     register    : <psm> [sec level]
     unregister  : [none]
     send        : <psm> <length of data>

Connection-oriented L2CAP
*************************

1. [Server] Register L2CAP Server:

When the Bluetooth stack has been initialized (:code:`bt init`), the L2CAP server can be registered
by calling :code:`br l2cap register`.

.. code-block:: console

   uart:~$ br l2cap register 1001 none
   L2CAP psm 4097 registered

2. [Client] Create L2CAP connection:

The command can only be used after the ACL connection has been established.

.. code-block:: console

   uart:~$ br l2cap connect 1001 none
   L2CAP connection pending

3. L2CAP connection is established:

.. code-block:: console

   uart:~$
   Security changed: XX:XX:XX:XX:XX:XX level 2
   Incoming BR/EDR conn 0x20004848
   Channel 0x20000b18 connected
   It is basic mode

3. Send L2CAP data to remote:

.. code-block:: console

   uart:~$ br l2cap send
   Rem 0

4. L2CAP data is received:

.. code-block:: console

   uart:~$
   Incoming data channel 0x20000b18 len 200
   00000000: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   00000010: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   00000020: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   00000030: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   00000040: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   00000050: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   00000060: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   00000070: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   00000080: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   00000090: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   000000A0: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   000000B0: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 |........ ........|
   000000C0: 00 00 00 00 00 00 00 00                          |........         |

5. Disconnect L2CAP connection:

.. code-block:: console

   uart:~$ br l2cap disconnect

6. L2CAP connection is broken:

.. code-block:: console

   Channel 0x20000b18 disconnected


L2CAP echo
**********

The echo subcommand provides functionality for L2CAP echo requests and responses in Bluetooth
Classic.

The commands can only be used after the ACL connection has been established.

1. Listen for L2CAP echo request an L2CAP echo response:

.. code-block:: console

   uart:~$ br l2cap echo register

2. Stop listening for L2CAP echo request an L2CAP echo response:

.. code-block:: console

   uart:~$ br l2cap echo unregister

3. Send L2CAP echo request:

.. code-block:: console

   uart:~$ br l2cap echo req 1

4. Echo request is received:

.. code-block:: console

   Incoming ECHO REQ data identifier 4 len 1
   00000000: 00                                               |.                |

5. Send L2CAP echo response:

.. code-block:: console

   uart:~$ br l2cap echo rsp 4 1

6. Echo response is received:

.. code-block:: console

   uart:~$
   Incoming ECHO RSP data len 1
   00000000: 00                                               |.                |


Connectionless L2CAP
********************

The connless subcommand provides functionality for connectionless L2CAP communication in Bluetooth
Classic, allowing packet transmission without establishing a L2CAP connection.

The subcommand is controlled by :kconfig:option:`CONFIG_BT_L2CAP_CONNLESS`.

The commands can only be used after the ACL connection has been established.

1. Listen for connectionless L2CAP packets:

.. code-block:: console

   uart:~$ br l2cap connless register 1001
   Register connectionless callbacks with PSM 0x1001

2. Send data

.. code-block:: console

   uart:~$ br l2cap connless send 1001 1
   Sending connectionless data with PSM 0x1001

3. Connectionless data is received:

.. code-block:: console

   Incoming connectionless data psm 0x1001 len 1
   00000000: 00                                               |.                |
