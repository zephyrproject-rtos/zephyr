.. zephyr:code-sample:: tftp-client
   :name: TFTP client
   :relevant-api: tftp_client

   Use the TFTP client library to get/put files from/to a TFTP server.

Overview
********

Trivial File Transfer Protocol (TFTP) is a simple lockstep File Transfer Protocol
based on UDP, and is designed to get a file from or put a file onto a remote host.

This TFTP client sample application for Zephyr implements the TFTP client library
and establishes a connection to a TFTP server on standard port 69.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/tftp_client`.

Requirements
************

- :ref:`networking_with_eth_qemu`, :ref:`networking_with_qemu` or :ref:`networking_with_native_sim`
- Linux machine

Building and Running
********************

There are configuration files for various setups in the
samples/net/tftp_client directory:

- :file:`prj.conf`
  This is the standard default config.

Build the tftp-client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/tftp_client
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

The easiest way to setup this sample application is to build and run it
as a native_sim application or as a QEMU target using the default configuration :file:`prj.conf`.
This requires a small amount of setup described in :ref:`networking_with_eth_qemu`, :ref:`networking_with_qemu` and :ref:`networking_with_native_sim`.

Build the tftp-client sample application for :ref:`native_sim <native_sim>` like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/tftp_client
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

Download and run a TFTP server (like TFTPd), then create file1.bin (with data) and newfile.bin.

Please note that default IP server address is 192.0.2.2 and default port is 69.
To specify an IP server address and/or port, change configurations in ``prj.conf``::

    CONFIG_TFTP_APP_SERVER="10.0.0.10"
    CONFIG_TFTP_APP_PORT="70"

To connect to server using hostname, enable DNS resolver by changing these two
configurations in ``prj.conf``::

    CONFIG_DNS_RESOLVER=y
    CONFIG_TFTP_APP_SERVER="my-tftp-server.org"

Sample output
==================================

Sample run on native_sim platform with TFTP server on host machine
Launch net-setup.sh in net-tools
.. code-block:: console

   net-setup.sh

.. code-block:: console

    <inf> net_config: Initializing network
    <inf> net_config: IPv4 address: 192.0.2.1
    <inf> net_tftp_client_app: Run TFTP client
    <inf> net_tftp_client_app: Received data:
            74 65 73 74 74 66 74 70  66 6f 72 7a 65 70 68 79 |testtftp forzephy
            72 0a                                            |r.
    <inf> net_tftp_client_app: TFTP client get done
    <inf> net_tftp_client_app: TFTP client put done
