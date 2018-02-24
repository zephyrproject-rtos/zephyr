.. _sockets-big-http-download:

Socket Big HTTP Download Example
################################

Overview
********

The sockets/big_http_download sample application for Zephyr implements
a simple HTTP GET client using a BSD Sockets compatible API. Unlike
the `sockets-http-get` sample application, it downloads a file of
several megabytes in size, and verifies its integrity using hashing. It
also performs download repeatedly, tracking the total number of bytes
transferred. Thus, it can serve as a "load testing" application for
the Zephyr IP stack.

The source code for this sample application can be found at:
:file:`samples/net/sockets/big_http_download`.

Requirements
************

- :ref:`networking_with_qemu`
- or, a board with hardware networking
- NAT/routing should be set up to allow connections to the Internet
- DNS server should be available on the host to resolve domain names

Building and Running
********************

Build the Zephyr version of the application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/big_http_download
   :board: <board_to_use>
   :goals: build
   :compact:

``board_to_use`` defaults to ``qemu_x86``. In this case, you can run the
application in QEMU using ``make run``. If you used another BOARD, you
will need to consult its documentation for application deployment
instructions. You can read about Zephyr support for specific boards in
the documentation at :ref:`boards`.

After the sample starts, it issues an HTTP GET request for
http://archive.ubuntu.com/ubuntu/dists/xenial/main/installer-amd64/current/images/hd-media/vmlinuz
This site was selected as providing files of variety of sizes, together
with various hashes for them, to ease external selection and verification.
The particular file selected is 6.7MB in size, so it can show how reliably
Zephyr streams non-trivial amounts of data, while still taking a
reasonable amount of time to complete. While the file is downloaded, its
hash is computed (SHA-256 is used in the source code), and an error
message is printed if it differs from the reference value, as specified
in the source code. After a short pause, the process repeats (in an
infinite loop), while the total counter of the bytes received is kept.
Thus the application can be used to test transfers of much larger amounts
of traffic over a longer time.

You can edit the source code to issue a request to any other site on
the Internet (or on the local network, in which case no NAT/routing
setup is needed).

.. warning::

   If you are doing extensive testing with this sample, please reference
   a file on a local server or a special-purpose testing server of your own
   on the Internet.  Using files on archive.ubuntu.com is not recommended for
   large-scale testing.

Running application on POSIX Host
=================================

The same application source code can be built for a POSIX system, e.g.
Linux.

To build for a host POSIX OS:

.. code-block:: console

    $ make -f Makefile.posix

To run:

.. code-block:: console

    $ ./big_http_download

The behavior of the application is the same as the Zephyr version.
