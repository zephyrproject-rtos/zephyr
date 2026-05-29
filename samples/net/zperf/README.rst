.. zephyr:code-sample:: zperf
   :name: zperf: Network Traffic Generator
   :relevant-api: net_config

   Use the zperf shell utility to evaluate network bandwidth.

Description
***********

The zperf sample demonstrates the :ref:`zperf shell utility <zperf>`, which
allows to evaluate network bandwidth.

Features
*********

- Compatible with iPerf v2.0.10 and newer. For older versions, enable
  :kconfig:option:`CONFIG_NET_ZPERF_LEGACY_HEADER_COMPAT`.

- Client or server mode allowed without need to modify the source code.

Supported Boards
****************

zperf is board-agnostic. However, to run the zperf sample application,
the target platform must provide a network interface supported by Zephyr.

This sample application has been tested on the following platforms:

- Freedom Board (FRDM K64F)
- QEMU x86
- Arm FVP BaseR AEMv8-R
- ARM BASE RevC AEMv8A Fixed Virtual Platforms

For best performance, the sample configures a lot of network packets and buffers.
Because of this, the sample's RAM requirements are quite large. In case the
sample does not fit into target platform RAM, reduce the following configs:

.. code-block:: cfg

   CONFIG_NET_PKT_RX_COUNT=40
   CONFIG_NET_PKT_TX_COUNT=40
   CONFIG_NET_BUF_RX_COUNT=160
   CONFIG_NET_BUF_TX_COUNT=160

Requirements
************

- iPerf 2.0.10 or newer installed on the host machine
- Supported board

Depending on the network technology chosen, extra steps may be required
to setup the network environment.

Usage
*****

See :ref:`zperf library documentation <zperf>` for more information about
the library usage.

Wi-Fi
=====

The IPv4 Wi-Fi support can be enabled in the sample with
:ref:`Wi-Fi snippet <snippet-wifi-ipv4>`.
