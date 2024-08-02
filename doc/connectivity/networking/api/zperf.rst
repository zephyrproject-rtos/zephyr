.. _zperf:

zperf: Network Traffic Generator
################################

.. contents::
    :local:
    :depth: 2

Overview
********

zperf is a shell utility which allows to generate network traffic in Zephyr. The
tool may be used to evaluate network bandwidth.

zperf is compatible with iPerf_2.0.5. Note that in newer iPerf versions,
an error message like this is printed and the server reported statistics
are missing.

.. code-block:: console

   LAST PACKET NOT RECEIVED!!!

zperf can be enabled in any application, a dedicated sample is also present
in Zephyr. See :zephyr:code-sample:`zperf sample application <zperf>` for details.

Sample Usage
************

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
