Bluetooth: Common Audio Profile Shell
#####################################

This document describes how to run the Common Audio Profile functionality.

CAP Acceptor
************

The Acceptor will typically be a resource-constrained device, such as a headset, earbud or hearing
aid. The Acceptor can initialize a Coordinated Set Identification Service instance, if it is in
a pair with one or more other CAP Acceptors.

Using the CAP Acceptor
======================

When the Bluetooth stack has been initialized (:code:`bt init`), the Acceptor can be registered by
by calling :code:`cap_acceptor init`, which will register the CAS and CSIS services, as well as
register callbacks.

.. code-block:: console


   cap_acceptor --help
   cap_acceptor - Bluetooth CAP acceptor shell commands
   Subcommands:
     init          :Initialize the service and register callbacks [size <int>]
                    [rank <int>] [not-lockable] [sirk <data>]
     lock          :Lock the set
     release       :Release the set [force]
     print_sirk    :Print the currently used SIRK
     set_sirk_rsp  :Set the response used in SIRK requests <accept, accept_enc,
                    reject, oob>

Besides initializing the CAS and the CSIS, there are also commands to lock and release the CSIS
instance, as well as printing and modifying access to the SIRK of the CSIS.


CAP Initiator
*************

The Initiator will typically be a resource-rich device, such as a phone or PC. The Initiator can
discover CAP Acceptors's CAS and optional CSIS services. The CSIS service can be read to provide
information about other CAP Acceptors in the same Coordinated Set. The Initiator can execute
stream control procedures on sets of devices, either ad-hoc or Coordinated, and thus provides an
easy way to setup multiple streams on multiple devices at once.

Using the CAP Initiator
=======================

When the Bluetooth stack has been initialized (:code:`bt init`), the Initiator can discover CAS and
the optionally included CSIS instance by calling (:code:`cap_initiator discover`).

.. code-block:: console

   cap_initiator --help
   cap_initiator - Bluetooth CAP initiator shell commands
   Subcommands:
     discover        :Discover CAS
     unicast-start   :Unicast Start [csip] [sinks <cnt> (default 1)] [sources <cnt>
                      (default 1)] [conns (<cnt> | all) (default 1)]
     unicast-list    :Unicast list streams
     unicast-update  :Unicast Update <all | stream [stream [stream...]]>
     unicast-stop    :Unicast stop all streams

Before being able to perform any stream operation, the device must also perform the
:code:`bap discover` operation to discover the ASEs and PAC records. The :code:`bap init`
command also needs to be called.

When connected
--------------

Discovering CAS and CSIS on a device:

.. code-block:: console

   uart:~$ cap_initiator discover
   discovery completed with CSIS


Discovering ASEs and PAC records on a device:

.. code-block:: console

   uart:~$ bap discover
   conn 0x81cc260: #0: codec 0x81d5b28 dir 0x01
   codec 0x06 cid 0x0000 vid 0x0000 count 5
   data #0: type 0x01 len 2
   00000000: f5                                               |.                |
   data #1: type 0x02 len 1
   data #2: type 0x03 len 1
   data #3: type 0x04 len 4
   00000000: 1e 00 f0                                         |...              |
   data #4: type 0x05 len 1
   meta #0: type 0x01 len 2
   00000000: 06                                               |.                |
   dir 1 loc 1
   snk ctx 6 src ctx 6
   Conn: 0x81cc260, Sink #0: ep 0x81e4248
   Conn: 0x81cc260, Sink #1: ep 0x81e46a8
   conn 0x81cc260: #0: codec 0x81d5f00 dir 0x02
   codec 0x06 cid 0x0000 vid 0x0000 count 5
   data #0: type 0x01 len 2
   00000000: f5                                               |.                |
   data #1: type 0x02 len 1
   data #2: type 0x03 len 1
   data #3: type 0x04 len 4
   00000000: 1e 00 f0                                         |...              |
   data #4: type 0x05 len 1
   meta #0: type 0x01 len 2
   00000000: 06                                               |.                |
   dir 2 loc 1
   snk ctx 6 src ctx 6
   Conn: 0x81cc260, Source #0: ep 0x81e5c88
   Conn: 0x81cc260, Source #1: ep 0x81e60e8
   Discover complete: err 0

Both of the above commands should be done for each device that you want to use in the set.
To use multiple devices, simply connect to more and then use :code:`bt select` the device to execute
the commands on.

Once all devices have been connected and the respective discovery commands have been called, the
:code:`cap_initiator unicast-start` command can be used to put one or more streams into the
streaming state.

.. code-block:: console

   uart:~$ cap_initiator unicast-start sinks 1 sources 0 conns all
   Setting up 1 sinks and 0 sources on each (2) conn
   Starting 1 streams
   Unicast start completed

To stop all the streams that has been started, the :code:`cap_initiator unicast-stop` command can be
used.


.. code-block:: console

   uart:~$ cap_initiator unicast-stop
   Unicast stopped for group 0x81e41c0 completed
