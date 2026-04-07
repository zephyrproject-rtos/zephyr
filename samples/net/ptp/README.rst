.. zephyr:code-sample:: ptp
   :name: PTP
   :relevant-api: ptp

   Enable PTP support and monitor messages and events via logging.

Overview
********

The PTP sample application for Zephyr will enable PTP (IEEE 1588-2019) support.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/ptp`.

Requirements
************

For generic host connectivity, that can be used for debugging purposes, see
:ref:`networking_with_native_sim` for details.

Building and Running
********************

A good way to run this sample is to run this PTP application inside
native_sim board as described in :ref:`networking_with_native_sim` or with
embedded device like Nucleo-H743-ZI, Nucleo-H745ZI-Q, Nucleo-F767ZI or
Nucleo-H563ZI. Note that PTP is only supported for
boards that have an Ethernet port and which has support for collecting
timestamps for sent and received Ethernet frames.

Follow these steps to build the PTP sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ptp
   :board: <board to use>
   :goals: build
   :compact:

The net-shell command ``net ptp`` will print general PTP information.
For detailed information about port 1, use ``net ptp 1`` (or ``net ptp port 1``).

Setting up Linux Host
=====================

By default PTP in Zephyr will not print any PTP debug messages to console.
One can enable debug prints by setting
:kconfig:option:`CONFIG_PTP_LOG_LEVEL_DBG` in the config file.

Get linuxptp project sources

.. code-block:: console

    git clone git://git.code.sf.net/p/linuxptp/code

Compile the ``ptp4l`` daemon and start it like this:

.. code-block:: console

    sudo ./ptp4l -4 -i zeth -m -q -l 6 -S

Or configure it with a config file like this:

.. code-block:: console
    [global]
    # Delay mechanism must match your configuration
    delay_mechanism        E2E

    # Use hardware timestamping if your NIC supports it
    time_stamping          hardware

    # Choose the transport that matches your network:
    # - L2 = EtherType 0x88f7 (typical in TSN/industrial, needs L2 reachability)
    # - UDPv4 = common in IT networks / routed segments
    network_transport      UDPv4

    # Must match switch domain
    domainNumber           0

    # Message rates (adjust as needed)
    logSyncInterval        -3
    logMinDelayReqInterval -3

    # Quality-of-life
    tx_timestamp_timeout   20
    summary_interval       1

and start it like this:

.. code-block:: console
    sudo ./ptp4l -i zeth -f /path/to/config/p2p.cfg -m

Compile Zephyr application.

.. zephyr-app-commands::
   :zephyr-app: samples/net/ptp
   :board: native_sim
   :goals: build
   :compact:

When the Zephyr image is build, you can start it like this:

.. code-block:: console

    build/zephyr/zephyr.exe -attach_uart

Interpreting net ptp Output
===========================

The shell provides two complementary views:

- ``net ptp``: instance-level summary (clock identity, current/parent dataset,
  best foreign time transmitter, and a per-port state table).
- ``net ptp <port>`` (for example ``net ptp 1``): detailed view for one port,
  including configuration and runtime counters/timers.

Example of ``net ptp`` output as a grandmaster:

.. code-block:: console

    uart:~$ net ptp
    PTP Instance:
    Clock ID      : 02:00:00:FF:FE:00:00:01
    Clock Type    : ORDINARY
    Domain        : 0
    Priorities    : 128 / 128
    Time source   : INTERNAL_OSCILLATOR (0xa0)
    Ports         : 1
    TR only       : no
    PHC now       : 95.703698720

    Current dataset:
    steps_rm      : 0
    offset_from_tt: 0 ns
    mean_delay    : 0 ns
    sync status   : stable

    Parent/GM dataset:
    grandmaster   : 02:00:00:FF:FE:00:00:01 (p1=128 p2=128 class=248 acc=0xfe)
    parent port   : 02:00:00:FF:FE:00:00:01-0
    UTC offset    : 37

    Best foreign transmitter : none

    Port  Interface  State
       1          1  GRAND_MASTER

Example of ``net ptp 1`` output as a grandmaster:

.. code-block:: console

    uart:~$ net ptp 1
    PTP Port 1:
    interface            : 1
    port identity        : 02:00:00:FF:FE:00:00:01-1

    Configuration:
    state                : GRAND_MASTER
    enabled              : yes
    time transmitter only: no
    announce log itv     : 1
    announce timeout     : 5
    sync log itv         : 0
    min delay_req log itv: 0
    delay mechanism      : E2E
    delay asymmetry      : 0 ns
    mean link delay      : 0 ns

    Runtime:
    seq announce/delay/signaling/sync: 66 / 0 / 0 / 132
    timeouts bits      : announce=0 delay=0 sync=0 qualification=0
    timer remaining ms : announce=1677 delay=0 sync=677 qualification=0
    PHC now            : 145.138244620
    best foreign       : none

Example of ``net ptp`` output as a time receiver:

.. code-block:: console

      uart:~$ net ptp
      PTP Instance:
      Clock ID      : 02:00:00:FF:FE:00:00:01
      Clock Type    : ORDINARY
      Domain        : 0
      Priorities    : 128 / 128
      Time source   : INTERNAL_OSCILLATOR (0xa0)
      Ports         : 1
      TR only       : no
      PHC now       : 1774472701.341945363

      Current dataset:
      steps_rm      : 2
      offset_from_tt: 22 ns
      mean_delay    : 639 ns
      sync status   : stable

      Parent/GM dataset:
      grandmaster   : 02:00:00:FF:FE:00:00:10 (p1=128 p2=128 class=6 acc=0x21)
      parent port   : 02:00:00:FF:FE:00:00:20-2
      UTC offset    : 37

      Best foreign transmitter : 02:00:00:FF:FE:00:00:20-2 (records=2)
      Dataset                : p1=128 p2=128 steps_rm=1 receiver=02:00:00:FF:FE:00:00:01-1

      Port  Interface  State
         1          1  TIME_RECEIVER

Example of ``net ptp 1`` output as a time receiver:

.. code-block:: console

    uart:~$ net ptp 1
    PTP Port 1:
    interface            : 1
    port identity        : 02:00:00:FF:FE:00:00:01-1

    Configuration:
    state                : TIME_RECEIVER
    enabled              : yes
    time transmitter only: no
    announce log itv     : 1
    announce timeout     : 5
    sync log itv         : 0
    min delay_req log itv: 0
    delay mechanism      : E2E
    delay asymmetry      : 0 ns
    mean link delay      : 0 ns

    Runtime:
    seq announce/delay/signaling/sync: 0 / 1268 / 0 / 0
    timeouts bits      : announce=0 delay=0 sync=0 qualification=0
    timer remaining ms : announce=9459 delay=539 sync=4408 qualification=0
    PHC now            : 1774472701.764088663
    best foreign       : 02:00:00:FF:FE:00:00:20-2 (p1=128 p2=128 steps_rm=1)


Useful fields in these views:

- ``TR only``: whether :kconfig:option:`CONFIG_PTP_TIME_RECEIVER_ONLY` is
  enabled.
- ``PHC now``: current hardware PTP clock time read from the Ethernet PHC.
- ``steps_rm``: number of network steps to the selected grandmaster.
- ``offset_from_tt`` and ``mean_delay``: current synchronization offset and path
  delay estimate.
- ``Best foreign transmitter``: best Announce candidate seen on the network for
  BMCA/state decision.

Role/state interpretation:

- ``GRAND_MASTER`` with ``best foreign : none`` means no eligible foreign
  transmitter has been selected yet, so the local clock is currently acting as
  grandmaster.
- Once valid Announce messages are received and selected, the state typically
  transitions to ``TIME_RECEIVER`` or ``PASSIVE`` depending on BMCA results.
