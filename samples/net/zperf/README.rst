zperf: Network Traffic Generator
################################

Description
***********

zperf is a network traffic generator for Zephyr that may be used to
evaluate network bandwidth.

Features
*********

- Compatible with iPerf_2.0.5.
- Client or server mode allowed without need to modify the source code.
- Working with task profiler (PROFILER=1 to be set when building zperf)

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


In the Zephyr console, zperf can be executed as follows:

.. code-block:: console

   zperf> udp.upload 2001:db8::2 5001 10 1K 1M


If the IP addresses of Zephyr and the host machine are specified in the
config file, zperf can be started as follows:

.. code-block:: console

   zperf> udp.upload2 v6 10 1K 1M


If Zephyr is acting as a server, set the download mode as follows:

.. code-block:: console

   zperf> udp.download 5001


and in the host side, iPerf must be executed with the following
command line:

.. code-block:: console

   $ iperf -l 1K -u -V -c 2001:db8::1 -p 5001


iPerf output can be limited by using the -b option if Zephyr is not
able to receive all the packets in orderly manner.
