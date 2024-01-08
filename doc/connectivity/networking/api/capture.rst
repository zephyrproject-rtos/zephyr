.. _net_capture_interface:

Network Packet Capture
######################

.. contents::
    :local:
    :depth: 2

Overview
********

The ``net_capture`` API allows user to monitor the network
traffic in one of the Zephyr network interfaces and send that traffic to
external system for analysis. The monitoring can be setup either manually
using ``net-shell`` or automatically by using the ``net_capture`` API.

Sample usage
************

See :zephyr:code-sample:`net-capture` sample application and
:ref:`network_monitoring` for details.


API Reference
*************

.. doxygengroup:: net_capture
