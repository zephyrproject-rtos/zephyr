.. _zperf-sample:

zperf: Network Traffic Generator
################################

Description
***********

zperf is a network traffic generator for Zephyr that may be used to
evaluate network bandwidth.

Features
*********

- Compatible with iPerf_2.0.9. Note that UDP uploader (client) testing did
  not work properly in iPerf_2.0.13: an error message like this is printed
  and the server reported statistics are missing.

.. code-block:: console

   LAST PACKET NOT RECEIVED!!!

- Client or server mode allowed without need to modify the source code.

Supported Boards
****************

zperf is board-agnostic. However, to run the zperf sample application,
the target platform must provide a network interface supported by Zephyr.

This sample application has been tested on the following platforms:

- Freedom Board (FRDM K64F)
- Quark SE C1000 Development Board
- QEMU x86

Requirements
************

- iPerf 2.0.5 installed on the host machine
- Supported board

Depending on the network technology chosen, extra steps may be required
to setup the network environment.

Usage
*****

If Zephyr acts as a client, iPerf must be executed in server mode.
For example, the following command line must be used for UDP testing:

.. code-block:: console

   $ iperf -s -l 1K -u -V -B 2001:db8::2

For TCP testing, the command line would look like this:

.. code-block:: console

   $ iperf -s -l 1K -V -B 2001:db8::2


In the Zephyr console, zperf can be executed as follows:

.. code-block:: console

   zperf udp upload 2001:db8::2 5001 10 1K 1M


For TCP the zperf command would look like this:

.. code-block:: console

   zperf tcp upload 2001:db8::2 5001 10 1K 1M


If the IP addresses of Zephyr and the host machine are specified in the
config file, zperf can be started as follows:

.. code-block:: console

   zperf udp upload2 v6 10 1K 1M


or like this if you want to test TCP:

.. code-block:: console

   zperf tcp upload2 v6 10 1K 1M


If Zephyr is acting as a server, set the download mode as follows for UDP:

.. code-block:: console

   zperf udp download 5001


or like this for TCP:

.. code-block:: console

   zperf tcp download 5001


and in the host side, iPerf must be executed with the following
command line if you are testing UDP:

.. code-block:: console

   $ iperf -l 1K -u -V -c 2001:db8::1 -p 5001


and this if you are testing TCP:

.. code-block:: console

   $ iperf -l 1K -V -c 2001:db8::1 -p 5001


iPerf output can be limited by using the -b option if Zephyr is not
able to receive all the packets in orderly manner.
