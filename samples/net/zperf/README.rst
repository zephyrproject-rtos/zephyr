Zperf: Network Traffic Generator
################################

Description
===========

Zperf is a network traffic generator for Zephyr that may be used to
evaluate network bandwidth.

Features
=========

- Compatible with iPerf_2.0.5.
- Client or server mode allowed without need to modify the source code.
- Working with task profiler (PROFILER=1 to be set when building zperf)

Supported Boards
================

zperf is board-agnostic. However, zperf requires a network interface.
So far, zperf has been tested only on the Intel Galileo Development Board.

Usage
=====

If the zephyr is a client, then you can start the iperf in host with these
command line options if you want to test UDP:

.. code-block:: console

   $ iperf -s -l 1K -u -V -B 2001:db8::2

In zephyr start zperf like this

.. code-block:: console

   zperf> udp.upload 2001:db8::2 5001 10 1K 1M

or if you have set the zephyr and peer host IP addresses in config file,
then you can simply say

.. code-block:: console

   zperf> udp.upload2 v6 10 1K 1M


If the zephyr is acting as a server, then first start zephyr in download
mode like this:

.. code-block:: console

   zperf> udp.download 5001

and in host side start iperf like this

.. code-block:: console

   $ iperf -l 1K -u -V -c 2001:db8::1 -p 5001

Note the you might need to rate limit the output using -b option
if zephyr is not able to receive all the packets in orderly manner.
