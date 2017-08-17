.. _async-sockets-echo-sample:

Asynchronous Socket Echo Server
###############################

Overview
********

The sockets/echo-async sample application for Zephyr implements an
asyncronous IPv4 TCP echo server using a BSD Sockets compatible API with
non-blocking sockets and a ``poll()`` call. This is an extension of
the :ref:`sockets-echo-sample` sample. The purpose of this sample is
still to show how it's possible to develop a sockets application portable
to both POSIX and Zephyr. As such, it is kept minimal and supports only
IPv4 and TCP.

The source code for this sample application can be found at:
:file:`samples/net/sockets/echo_async`.

Requirements
************

- :ref:`networking_with_qemu`
- or, a board with hardware networking

Building and Running
********************

Build the Zephyr version of the sockets/echo_async application like this:

.. code-block:: console

    $ cd $ZEPHYR_BASE/samples/net/sockets/echo_async
    $ make pristine
    $ make BOARD=<board_to_use>

``board_to_use`` defaults to ``qemu_x86``. In this case, you can run the
application in QEMU using ``make run``. If you used another BOARD, you
will need to consult its documentation for application deployment
instructions. You can read about Zephyr support for specific boards in
the documentation at :ref:`boards`.

After the sample starts, it expects connections at 192.0.2.1, port 4242.
The easiest way to connect is:

.. code-block:: console

    $ telnet 192.0.2.1 4242

After a connection is made, the application will echo back any line sent to
it. Unlike the above-mentioned :ref:`sockets-echo-sample`, this application
supports multiple concurrent client connections. You can open
another terminal window and run the same telnet command as above.
The sample supports up to three connected clients, but this can be adjusted
by changing ``NUM_FDS`` defined in the source code.

Running application on POSIX Host
=================================

The same application source code can be built for a POSIX system, e.g.
Linux. (Note: if you look at the source, you will see that the code is
the same except the header files are different for Zephyr vs POSIX.)

To build for a host POSIX OS:

.. code-block:: console

    $ make -f Makefile.posix

To run:

.. code-block:: console

    $ ./socket_echo

To test:

.. code-block:: console

    $ telnet 127.0.0.1 4242

As can be seen, the behavior of the application is the same as the Zephyr
version.
