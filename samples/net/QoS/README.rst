.. zephyr:code-sample:: quality-of-service
   :name: Quality of Service
   :relevant-api: bsd_sockets

   Implements a simple demo of quality of service.

Overview
********

The purpose of this sample is to a basic example of quality-of-service.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/QoS`.

Building and Running
********************

Build the Zephyr version of the sockets/echo_service application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/QoS
   :board: <board_to_use>
   :goals: build
   :compact:

After the sample starts, it runs fakes the reception of a series of packets
and prints statistics about it.

Run with `west build -b native_sim && ./build/zephyr/zephyr.exe`

--- Statistics with dynamic priority assignment ---

work us | service_123 | service_124 | service_125 | service_126 | service_127 | service_128 | service_129 | service_130
   2000 |          50 |          50 |          50 |          50 |          50 |          50 |          50 |          50
   3000 |           1 |          34 |          50 |          50 |          50 |          50 |          50 |          50
   4000 |           1 |           1 |           1 |          50 |          50 |          50 |          50 |          50
   6000 |           1 |           1 |           1 |           1 |          18 |          50 |          50 |          50
  10000 |           1 |           1 |           1 |           1 |           1 |          34 |          34 |          34
  25000 |           1 |           1 |           1 |           1 |           1 |           1 |          16 |          25


--- Statistics without dynamic priority assignment (commented out `update_priority_l2` in `net_core.c`) ---

work us | service_123 | service_124 | service_125 | service_126 | service_127 | service_128 | service_129 | service_130
   2000 |          34 |          34 |          34 |          34 |          34 |          34 |          34 |          34
   3000 |          25 |          25 |          25 |          25 |          25 |          25 |          25 |          25
   4000 |          20 |          20 |          20 |          20 |          20 |          20 |          20 |          20
   6000 |          17 |          17 |          17 |          17 |          17 |          17 |          17 |          17
  10000 |          10 |          10 |          10 |          10 |          10 |          10 |          10 |          10
  25000 |           5 |           5 |           5 |           5 |           5 |           5 |           5 |           5


