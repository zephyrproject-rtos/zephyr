.. _async-sockets-echo-select-sample:

Asynchronous Socket Echo Server Using select()
##############################################

Overview
********

The sockets/echo_async_select sample application for Zephyr implements an
asynchronous IPv4/IPv6 TCP echo server using a BSD Sockets compatible API
with non-blocking sockets and a ``select()`` call. This is a variant of
the :ref:`async-sockets-echo-sample` sample.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/echo_async_select`.

Requirements
************

- :ref:`networking_with_host`
- or, a board with hardware networking (including 6LoWPAN)

Building and Running
********************

Build the Zephyr version of the sockets/echo_async application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_async_select
   :board: <board_to_use>
   :goals: build
   :compact:

After the sample starts, it expects connections at 192.0.2.1 (IPv4), or
2001:db8::1 (IPv6), port 4242. The easiest way to connect is:

.. code-block:: console

    $ telnet 192.0.2.1 4242     # use this for IPv4
    $ telnet 2001:db8::1 4242   # or this for IPv6

After a connection is made, the application will echo back any line sent to
it. Unlike the :ref:`sockets-echo-sample`, this application
supports multiple concurrent client connections. You can open
another terminal window and run the same telnet command as above.
The sample supports up to three connected clients, but this can be adjusted
by changing ``NUM_FDS`` defined in the source code.

Running application on POSIX Host
=================================

The same application source code can be built for a POSIX system, e.g.
Linux. (Note: if you look at the source, you will see that the code is
the same except the header files are different for Zephyr vs POSIX, and
there's an additional option to set for Linux to make a socket IPv6-only).

To build for a host POSIX OS:

.. code-block:: console

    $ make -f Makefile.posix

To run:

.. code-block:: console

    $ ./socket_echo

To test:

.. code-block:: console

    $ telnet 127.0.0.1 4242   # use this for IPv4
    $ telnet ::1 4242         # or this for IPv6

As can be seen, the behavior of the application is the same as the Zephyr
version.
