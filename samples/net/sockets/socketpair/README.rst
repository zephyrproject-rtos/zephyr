.. _sockets-socketpair-sample:

Socketpair Example
##################

Overview
********

The sockets/socketpair sample application for Zephyr demonstrates a
multi-threaded application communicating over pairs of unnamed,
connected UNIX-domain sockets. The pairs of sockets are created with
socketpair(2), as you might have guessed. Such sockets are compatible
with the BSD Sockets API, and therefore the purpose of this sample
is also to reinforce that it is possible to develop a sockets
application portable to both POSIX and Zephyr.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/socketpair`.

Requirements
************

None

Building and Running
********************

Build the Zephyr version of the sockets/echo application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/socketpair
   :board: <board_to_use>
   :goals: build
   :compact:

After the sample starts, several clients thread are spawned and each client
thread sends a fixed number of messages to the server (main). Each client
sends a message (it's name) to the server.

.. code-block:: console

    *** Booting Zephyr OS build v2.3.0-rc1-215-g0e36f9686836  ***
    Alpha: socketpair: 3 <=> 4
    Bravo: socketpair: 5 <=> 7
    Charlie: socketpair: 8 <=> 9
    Charlie closed fd 9
    fd: 8: read 21 bytes
    fd: 8: hung up
    main: closed fd 8
    joined Charlie
    Alpha closed fd 4
    fd: 3: read 15 bytes
    fd: 3: hung up
    main: closed fd 3
    joined Alpha
    Bravo closed fd 7
    fd: 5: read 15 bytes
    fd: 5: hung up
    main: closed fd 5
    joined Bravo
    finished!

Running application on POSIX Host
=================================

The same application source code can be built for a POSIX system, e.g.
Linux.

To build for a host POSIX OS:

.. code-block:: console

    $ make -f Makefile.posix

To run:

.. code-block:: console

    $ ./socketpair_example
    Alpha: socketpair: 3 <=> 4
    Bravo: socketpair: 5 <=> 6
    Charlie: socketpair: 7 <=> 8
    Alpha closed fd 4
    fd: 3: read 15 bytes
    fd: 3: hung up
    main: closed fd 3
    joined Alpha
    fd: 5: read 15 bytes
    fd: 5: hung up
    Bravo closed fd 6
    main: closed fd 5
    joined Bravo
    Charlie closed fd 8
    fd: 7: read 21 bytes
    fd: 7: hung up
    main: closed fd 7
    joined Charlie
    finished!

As can be seen, the behavior of the application is approximately the same as
the Zephyr version.
