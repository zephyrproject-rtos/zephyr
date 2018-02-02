.. _throughput-server-sample:

Throughput Server
#################

Overview
********

The throughput-server sample application for Zephyr implements a UDP server
that complements the throughput-client tool from Zephyr `net-tools`_ repository.
The throughput-server listens for incoming IPv4 or IPv6 packets (sent by the
throughput-client) and optionally sends the packets back.
Note that it makes sense to use this sample (only) with high-performance
bearer like Ethernet.

The source code for this sample application can be found at:
:file:`samples/net/throughput_server`.

Building and Running
********************

The application can be used on multiple boards and with different bearers.
Emulation using QEMU is not recommended because the ethernet connection in
QEMU is done using a serial port with SLIP, so it is very slow.

There are configuration files for different boards and setups in the
throughput-server directory:

- :file:`prj_frdm_k64f.conf`
  Use this for FRDM-K64F board with built-in ethernet.

Build throughput-server sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/throughput_server
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Make will select the default configuration file based on the BOARD you
specified to CMake.

Note that shell support is not activated in the throughput-server so that
we can save some memory and have as many net_buf's configured in the system
as possible.

Running throughput-client in Linux Host
=======================================

In this example, the throughput-server is run on a board and throughput-client
is run on a Linux host. The throughput-client can be found at the `net-tools`_
project.

Open a terminal window and type:

.. code-block:: console

    $ cd net-tools
    $ ./throughput-client -F -s 200 2001:db8::1

Note that throughput-server must be running on the device under test before you
start the throughput-client application in a host terminal window.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
