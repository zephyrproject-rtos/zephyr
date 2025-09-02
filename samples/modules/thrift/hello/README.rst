.. zephyr:code-sample:: thrift-hello
   :name: Apache Thrift Hello World

   Implement a simple Apache Thrift client-server application.

Overview
********

.. figure:: thrift-layers.png
   :align: center
   :alt: Thrift Layers

What is Thrift?
***************

`Apache Thrift`_ is an `IDL`_ specification, `RPC`_ framework, and
`code generator`_. It works across all major operating systems, supports over
27 programming languages, 7 protocols, and 6 low-level transports. Thrift was
originally developed at `Facebook in 2006`_ and then shared with the
`Apache Software Foundation`_. Thrift supports a rich set of types and data
structures, and abstracts away transport and protocol details, which lets
developers focus on application logic.

.. _Apache Thrift:
    https://github.com/apache/thrift

.. _IDL:
    https://en.wikipedia.org/wiki/Interface_description_language

.. _RPC:
    https://en.wikipedia.org/wiki/Remote_procedure_call

.. _code generator:
    https://en.wikipedia.org/wiki/Automatic_programming

.. _Facebook in 2006:
    https://thrift.apache.org/static/files/thrift-20070401.pdf

.. _Apache Software Foundation:
    https://www.apache.org

Overview
********

This sample application includes a client and server implementing the RPC
interface described in :zephyr_file:`samples/modules/thrift/hello/hello.thrift`.
The purpose of this example is to demonstrate how components at different
layers in thrift can be combined to build an application with desired features.


Requirements
************

- Optional Modules

.. code-block:: console
   :caption: Download optional modules with west

   west config manifest.group-filter -- +optional
   west update

- QEMU Networking (described in :ref:`networking_with_qemu`)
- Thrift dependencies installed for your host OS

.. tabs::

   .. group-tab:: Ubuntu

      .. code-block:: bash
        :caption: Install thrift dependencies in Ubuntu

         sudo apt install -y libboost-all-dev thrift-compiler libthrift-dev

   .. group-tab:: macOS

      .. code-block:: bash
        :caption: Install thrift dependencies in macOS

         brew install boost openssl thrift

Building and Running
********************

This application can be run on a Linux host, with either the server or the
client in the QEMU environment, and the peer is built and run natively on
the host.

Building the Native Client and Server
=====================================

.. code-block:: console

   $ make -j -C samples/modules/thrift/hello/client/
   $ make -j -C samples/modules/thrift/hello/server/

Under ``client/``, 3 executables will be generated, and components
used in each layer of them are listed below:

+----------------------+------------+--------------------+------------------+
| hello_client         | TSocket    | TBufferedTransport | TBinaryProtocol  |
+----------------------+------------+--------------------+------------------+
| hello_client_compact | TSocket    | TBufferedTransport | TCompactProtocol |
+----------------------+------------+--------------------+------------------+
| hello_client_ssl     | TSSLSocket | TBufferedTransport | TBinaryProtocol  |
+----------------------+------------+--------------------+------------------+

The same applies for the server. Only the client and the server with the
same set of stacks can communicate.

Additionally, there is a ``hello_client.py`` Python script that can be used
interchangeably with the ``hello_client`` C++ application to illustrate the
cross-language capabilities of Thrift.

+----------------------+------------+--------------------+------------------+
| hello_client.py      | TSocket    | TBufferedTransport | TBinaryProtocol  |
+----------------------+------------+--------------------+------------------+

Running the Zephyr Server in QEMU
=================================

Build the Zephyr version of the ``hello/server`` sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/thrift/hello/server
   :board: board_name
   :goals: build
   :compact:

To enable advanced features, extra arguments should be passed accordingly:

- TCompactProtocol: ``-DCONFIG_THRIFT_COMPACT_PROTOCOL=y``
- TSSLSocket: ``-DCONF_FILE="prj.conf ../overlay-tls.conf"``

For example, to build for ``qemu_x86_64`` with TSSLSocket support:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/thrift/hello/server
   :host-os: unix
   :board: qemu_x86_64
   :conf: "prj.conf ../overlay-tls.conf"
   :goals: run
   :compact:

In another terminal, run the ``hello_client`` sample app compiled for the
host OS:

.. code-block:: console

    $ ./hello_client 192.0.2.1
    $ ./hello_client_compact 192.0.2.1
    $ ./hello_client_ssl 192.0.2.1 ../native-cert.pem ../native-key.pem ../qemu-cert.pem

You should observe the following in the original ``hello/server`` terminal:

.. code-block:: console

    ping
    echo: Hello, world!
    counter: 1
    counter: 2
    counter: 3
    counter: 4
    counter: 5

In the client terminal, run ``hello_client.py`` app under the host OS (not
described for compact or ssl variants for brevity):

.. code-block:: console

    $ ./hello_client.py

You should observe the following in the original ``hello/server`` terminal.
Note that the server's state is not discarded (the counter continues to
increase).

.. code-block:: console

    ping
    echo: Hello, world!
    counter: 6
    counter: 7
    counter: 8
    counter: 9
    counter: 10

Running the Zephyr Client in QEMU
=================================

In another terminal, run the ``hello_server`` sample app compiled for the
host OS:

.. code-block:: console

    $ ./hello_server 0.0.0.0
    $ ./hello_server_compact 0.0.0.0
    $ ./hello_server_ssl 0.0.0.0 ../native-cert.pem ../native-key.pem ../qemu-cert.pem


Then, in another terminal, run the corresponding ``hello/client`` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/thrift/hello/client
   :board: qemu_x86_64
   :goals: run
   :compact:

The additional arguments for advanced features are the same as
``hello/server``.

You should observe the following in the original ``hello_server`` terminal:

.. code-block:: console

    ping
    echo: Hello, world!
    counter: 1
    counter: 2
    counter: 3
    counter: 4
    counter: 5
