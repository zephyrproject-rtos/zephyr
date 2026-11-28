.. zephyr:code-sample:: canopen
   :name: CANopen node
   :relevant-api: canopen

   Run a CANopen node with PDOs and access it from a CANopen master.

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

The node consumes TIME messages. Once it has received one and is made TIME producer by setting
bit 30 of ``1012h``, it sends its clock once per second.

Requirements
************

* A board with a CAN controller and transceiver, and a CAN bus terminated with
  120 ohm at both ends.
* A CANopen master on the same bus, for example a Linux host with a SocketCAN
  interface and the ``can-utils`` tools.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/canbus/canopen
   :board: <board>
   :goals: build flash
   :compact:

On a Linux host, bring up the CAN interface with the same bitrate, watch the bus and start the
node with an NMT command:

.. code-block:: console

   sudo ip link set can0 up type can bitrate 500000
   candump can0 &
   cansend can0 000#0110

The node sends its heartbeat on CAN-ID 710h, ``05`` once it is operational, and TPDO1 with the
counter on CAN-ID 190h. An SDO upload of ``1017h`` returns the heartbeat producer time:

.. code-block:: console

   cansend can0 610#4017100000000000
     can0  590   [8]  4B 17 10 00 E8 03 00 00

Sample Output
=============

.. code-block:: console

   [00:00:00.010,000] <inf> canopen_sample: NMT state 0x7f
   [00:00:00.010,000] <inf> canopen_sample: CANopen node 0x10 started at 500000 bit/s
   [00:00:05.120,000] <inf> canopen_sample: NMT state 0x05
   [00:00:08.230,000] <inf> canopen_sample: RPDO1: output 0xa5
