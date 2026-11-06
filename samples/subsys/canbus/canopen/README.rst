.. zephyr:code-sample:: canopen
   :name: CANopen node
   :relevant-api: canopen

   Run a CANopen node with PDOs and check it from a Linux host with python-canopen.

Overview
********

This sample runs a CANopen node (CiA 301) on the CAN controller selected by the
``zephyr,canbus`` chosen node. The node provides these application objects:

* ``2000h``: UNSIGNED32 counter, read only, incremented every 100 ms and
  transmitted in TPDO1 (CAN-ID 180h + node-ID) every second.
* ``2001h``: UNSIGNED8 output, read and write, received with RPDO1 (CAN-ID
  200h + node-ID).

The node-ID (default 16), the bitrate (default 500 kbit/s), the counter period
and the TPDO1 event timer are set with the ``CONFIG_SAMPLE_*`` options. The
heartbeat producer time (``1017h``) is 1000 ms.

If the board defines the ``canopen-run-led`` and ``canopen-err-led`` aliases to
``gpio-leds`` nodes and :kconfig:option:`CONFIG_LED` is enabled, the node drives
them as the CiA 303-3 RUN and ERR LEDs.

Requirements
************

* A board with a CAN controller and transceiver, and a CAN bus terminated with
  120 ohm at both ends.
* A Linux host with a SocketCAN interface on the same bus, for example a
  Raspberry Pi with a CAN HAT or a USB-CAN adapter, with python-canopen
  installed (``pip install canopen``).

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/canbus/canopen
   :board: <board>
   :goals: build flash
   :compact:

On the Linux host, bring up the CAN interface with the same bitrate and run the
check script from this directory:

.. code-block:: console

   sudo ip link set can0 up type can bitrate 500000
   python3 canopen_master.py --channel can0 --node-id 16

The build also writes the electronic data sheet of the node to ``build/zephyr/canopen.eds``
(:kconfig:option:`CONFIG_CANOPEN_EDS`). With ``--eds build/zephyr/canopen.eds`` the script
additionally reads every readable object of the data sheet from the node and compares the
sizes and default values.

Shell
=====

Build with ``-DEXTRA_CONF_FILE=shell.conf`` to add the ``canopen`` shell command
(:kconfig:option:`CONFIG_CANOPEN_SHELL`) on the console, for example:

.. code-block:: console

   uart:~$ canopen list
   0x10 PRE-OPERATIONAL error register 0x00 can@40520200
   uart:~$ canopen read 16 0x1017 0
   1000 (0x03e8)
   uart:~$ canopen nmt 16 start
   uart:~$ canopen sdo read 16 0x20 0x1008 0

The overlay also enables the master functions, so ``canopen sdo`` and ``canopen nmt`` with a
target node-ID reach other nodes on the bus.

Master mode
===========

With :kconfig:option:`CONFIG_SAMPLE_MASTER` the node also acts as CANopen master. After
start-up it configures the slave with node-ID ``CONFIG_SAMPLE_SLAVE_ID`` (default 32) over SDO
(``1017h`` = 1000 ms, ``2000h`` = 11223344h), starts it with an NMT command, reads its device name
and monitors its heartbeat. On the Linux host, ``canopen_slave.py`` acts as that slave and checks
what the master did:

.. code-block:: console

   python3 canopen_slave.py --channel can0 --node-id 32

Start the script first, then reset the board.

Sample Output
=============

.. code-block:: console

   [00:00:00.010,000] <inf> canopen_sample: NMT state 0x7f
   [00:00:00.010,000] <inf> canopen_sample: CANopen node 0x10 started at 500000 bit/s
   [00:00:05.120,000] <inf> canopen_sample: NMT state 0x05
   [00:00:08.230,000] <inf> canopen_sample: RPDO1: output 0xa5

The script checks boot-up, NMT states and heartbeat, SDO transfers and abort codes, TPDO1 on
the event timer and on SYNC, RPDO1, and the error handling: an emergency message for a too
short RPDO, the error register and history, and the heartbeat consumer, which it configures to
monitor a heartbeat that the script sends and then stops. It also sends a TIME message and makes
the node TIME producer: the node sends its clock, set from the received time, once per second.
Finally it switches the heartbeat off, guards the node with remote frames and checks the toggle
bit, the life guarding emergency message when it stops and the error behavior.
It also writes 200 bytes to the DOMAIN object ``5600h``, reads them back and checks that a
download of 300 bytes is refused before any data is sent, then repeats the 200 bytes with SDO
block transfer and reads the device name ``1008h`` by block upload. It prints one line per check
and ``ALL CHECKS PASSED`` at the end.
