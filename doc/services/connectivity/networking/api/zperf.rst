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

zperf is compatible with iPerf 2.0.10 and newer. For compatibility with older versions,
enable :kconfig:option:`CONFIG_NET_ZPERF_LEGACY_HEADER_COMPAT`. Alternatively, zperf can speak
iperf3, see :ref:`zperf_iperf3`. A build speaks one of the two.

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

.. _zperf_iperf3:

iperf3
******

With :kconfig:option:`CONFIG_NET_ZPERF_IPERF3`, zperf interoperates with iperf3 instead of
iPerf 2. The support is experimental. The zperf API and shell commands stay the same, and the default port becomes 5201, the
iperf3 default.

When Zephyr acts as a server, enable the protocols to accept:

.. code-block:: console

   zperf tcp download
   zperf udp download

An iperf3 client chooses TCP or UDP for each test and uses one port for both, with a TCP control
connection either way. The two commands therefore share the listening port: each enables its own
protocol, and a test asking for one that is not enabled is refused. On the host:

.. code-block:: console

   $ iperf3 -c 2001:db8::1
   $ iperf3 -c 2001:db8::1 -u -b 10M

When Zephyr acts as a client, run ``iperf3 -s`` on the host, and in the Zephyr console:

.. code-block:: console

   zperf tcp upload 2001:db8::2 5201 10 1K
   zperf udp upload 2001:db8::2 5201 10 1K 10M

A test runs over a single stream. Reverse (``-R``), bidirectional (``--bidir``) and parallel
(``-P``) tests, and omitting the start of a test (``-O``), are refused with the message iperf3
prints for an option the server does not implement. One test runs at a time, and a second client
is told that the server is busy. Multicast and
:kconfig:option:`CONFIG_ZPERF_SESSION_PER_THREAD` are iPerf 2 features.

Points to keep in mind when comparing with iPerf 2:

* An iperf3 server counts TCP data until the client ends the test on the control connection,
  which can overtake the last of the data, so it may report slightly less than the client sent.
  An iPerf 2 server counts until the connection closes.
* For UDP, datagrams lost after the last one the server received are counted as lost in the zperf
  upload report, as with iPerf 2. iperf3 itself leaves them out.
* iperf3 does not report reordering, so an upload reports no out-of-order packets.
* The server reads only the header of each UDP datagram, relying on ``ZSOCK_MSG_TRUNC`` to learn
  its full length. A socket offload driver that ignores the flag makes it count too few bytes.
* A test takes a control connection and a data stream on each end, and closed connections linger
  in TIME_WAIT for a while after each test. Allow for them in
  :kconfig:option:`CONFIG_NET_MAX_CONTEXTS` and :kconfig:option:`CONFIG_NET_MAX_CONN`, as the
  sample's ``overlay-iperf3.conf`` does.

Session Management
******************

If :kconfig:option:`CONFIG_ZPERF_SESSION_PER_THREAD` option is set, then
multiple upload sessions can be done at the same time if user supplies ``-a``
option when starting the upload. Each session will have their own work queue
to run the test. The session test results can be viewed also after the tests
have finished. The sessions can be started with ``-w`` option which then
lets the worker threads to wait a start signal so that all the threads can
be started at the same time. This will prevent the case where the zperf shell
cannot run because it is running in lower priority than the already started
session thread. If you have only one upload session, then the ``-w`` is not
really needed.

Following zperf shell commands are available for session management:

.. csv-table::
   :header: "zperf shell command", "Description"
   :widths: auto

   "``jobs``", "Show currently active or finished sessions"
   "``jobs all``", "Show statistics of finished sessions"
   "``jobs clear``", "Clear finished session statistics"
   "``jobs start``", "Start all the waiting sessions"

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

The ``-w`` option can be used like this to delay the startup of the jobs.

.. code-block:: console

   uart:~$ zperf tcp upload -a -t 6 -w 192.0.2.2 5001 10 1K
   Remote port is 5001
   Connecting to 192.0.2.2
   Duration:       10.00 s
   Packet size:    1000 bytes
   Rate:           10 kbps
   Waiting "zperf jobs start" command.
   [01:06:51.392,288] <inf> net_zperf: [0] TCP waiting for start

   uart:~$ zperf udp upload -a -t 6 -w 192.0.2.2 5001 10 1K 10M
   Remote port is 5001
   Connecting to 192.0.2.2
   Duration:       10.00 s
   Packet size:    1000 bytes
   Rate:           10000 kbps
   Waiting "zperf jobs start" command.
   Rate:           10.00 Mbps
   Packet duration 781 us
   [01:06:58.064,552] <inf> net_zperf: [0] UDP waiting for start

   uart:~$ zperf jobs start
   -
   Upload completed!
   -
   Upload completed!

   # Note that the output may be garbled as two threads printed
   # output at the same time. Just print out the fresh listing
   # like this.

   uart:~$ zperf jobs all
   -
   Upload completed!
   Statistics:             server  (client)
   Duration:               9.99 s  (10.00 s)
   Num packets:            11429   (11429)
   Num packets out order:  0
   Num packets lost:       0
   Jitter:                 164 us
   Rate:                   9.14 Mbps       (9.14 Mbps)
   Thread priority:        6
   Protocol:               UDP
   Session id:             0
   -
   Upload completed!
   Duration:               10.00 s
   Num packets:            15487
   Num errors:             0 (retry or fail)
   Rate:                   12.38 Mbps
   Thread priority:        6
   Protocol:               TCP
   Session id:             0
   Total 2 sessions done

Custom Data Upload
******************

zperf supports more advanced data upload profiling by setting a custom data
source through :c:member:`zperf_upload_params.data_loader`. This enables the
generation of custom packet contents instead of sending a constant packet
consisting solely of the ``z`` character. An example use case would be
determining the maximum throughput of uploading data from an external flash
memory chip.

Raw TX Mode
***********

zperf supports raw packet transmission mode for testing custom L2 frames or
vendor-specific protocols. This mode bypasses UDP/TCP and sends raw packets
directly using packet sockets.

To enable raw TX mode, set the following Kconfig options:

.. code-block:: kconfig

   CONFIG_NET_SOCKETS_PACKET=y
   CONFIG_NET_ZPERF_RAW_TX=y

The maximum header size can be configured with
:kconfig:option:`CONFIG_NET_ZPERF_RAW_TX_MAX_HDR_SIZE` (default: 64 bytes).

Before using raw TX mode, TX injection mode must be enabled on the network
interface:

.. code-block:: console

   uart:~$ net iface txinjection 1 on

The raw TX upload command syntax is:

.. code-block:: console

   zperf raw upload [-a] <if_index> <header_hex> [<duration_sec>] [<packet_size>] [<rate_kbps>]

Where:

- ``-a``: Optional async mode flag
- ``if_index``: Network interface index (e.g., 1)
- ``header_hex``: User-provided header as hex string (vendor metadata + frame header)
- ``duration_sec``: Test duration in seconds (default: 1)
- ``packet_size``: Total packet size including header (default: 256)
- ``rate_kbps``: Target rate in Kbps, supports K/M suffixes (default: 10)

Example for sending raw 802.11 frames with vendor metadata:

.. code-block:: console

   uart:~$ zperf raw upload 1 12345678000400030000000088000000ffffffffffa06960e35215a06960e3521500000000aaaa030000000800 10 1024 50M

The header hex string contains:

- Vendor metadata (first 12 bytes in this example)
- 802.11 QoS Data header with LLC/SNAP

The payload (filled with ``z`` characters) is automatically appended to reach
the specified packet size.
