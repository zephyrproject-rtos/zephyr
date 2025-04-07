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

zperf is compatible with iPerf 2.0.10 and newer. For compatability with older versions,
enable :kconfig:option:`CONFIG_NET_ZPERF_LEGACY_HEADER_COMPAT`.

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

Session Management
******************

If :kconfig:option:`CONFIG_ZPERF_SESSION_PER_THREAD` option is set, then
multiple upload sessions can be done at the same time if user supplies ``-a``
option when starting the upload. Each session will have their own work queue
to run the test. The session test results can be viewed also after the tests
have finished.

Following zperf shell commands are available for session management:

.. csv-table::
   :header: "zperf shell command", "Description"
   :widths: auto

   "``jobs``", "Show currently active or finished sessions"
   "``jobs all``", "Show statistics of finished sessions"
   "``jobs clear``", "Clear finished session statistics"

Example:

.. code-block:: console

   uart:~$ zperf udp upload -a -t 5 192.0.2.2 5001 10 1K 1M
   Remote port is 5001
   Connecting to 192.0.2.2
   Duration:       10.00 s
   Packet size:    1000 bytes
   Rate:           1000 kbps
   Starting...
   Rate:           1.00 Mbps
   Packet duration 7 ms

   uart:~$ zperf jobs all
   No sessions sessions found
   uart:~$ zperf jobs
              Thread    Remaining
   Id  Proto  Priority  time (sec)
   [1] UDP    5            4

   Active sessions have not yet finished
   -
   Upload completed!
   Statistics:             server  (client)
   Duration:               30.01 s (30.01 s)
   Num packets:            3799    (3799)
   Num packets out order:  0
   Num packets lost:       0
   Jitter:                 63 us
   Rate:                   1.01 Mbps       (1.01 Mbps)
   Thread priority:        5
   Protocol:               UDP
   Session id:             1

   uart:~$ zperf jobs all
   -
   Upload completed!
   Statistics:             server  (client)
   Duration:               30.01 s (30.01 s)
   Num packets:            3799    (3799)
   Num packets out order:  0
   Num packets lost:       0
   Jitter:                 63 us
   Rate:                   1.01 Mbps       (1.01 Mbps)
   Thread priority:        5
   Protocol:               UDP
   Session id:             1
   Total 1 sessions done

   uart:~$ zperf jobs clear
   Cleared data from 1 sessions

   uart:~$ zperf jobs
   No active upload sessions
   No finished sessions found
