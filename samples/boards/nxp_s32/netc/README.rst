.. _nxp_s32_netc-samples:

NXP S32 NETC Sample Application
###############################

Overview
********

The sample application shows how to configure NXP S32 Network Controller (NETC)
for the different use-cases:

1. Zephyr application controls the Physical Station Interface (PSI) and the
   Ethernet PHY through EMDIO.

2. Zephyr application controls the PSI, Virtual SI 1, and the Ethernet PHY
   through EMDIO.

The sample enables the net-shell and mdio-shell (only available when Zephyr
controls PSI) to allow users visualize the networking settings. Telnet shell
and backend is also enabled.

The source code for this sample application can be found at:
:zephyr_file:`samples/boards/nxp_s32/netc`.

Requirements
************

To run this sample is needed to set-up a host machine running GNU/Linux or Windows
with a network adapter connected to the target board ETH0 port through an Ethernet
cable.

Building and Running
********************

To build and run the sample application for use-case 1:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp_s32/netc
   :board: s32z2xxdc2/s32z270/rtu0
   :goals: build flash

Once started, you should see the network interface details, for example:

.. code-block:: console

   [00:00:00.051,000] <inf> phy_mii: PHY (7) ID 1CC916

   [00:00:00.052,000] <inf> nxp_s32_eth_psi: SI0 MAC: 00:00:00:01:02:00
   [00:00:00.058,000] <inf> shell_telnet: Telnet shell backend initialized
   [00:00:00.058,000] <inf> nxp_s32_netc_sample: Starting sample
   [00:00:00.058,000] <inf> nxp_s32_netc_sample: Waiting for iface 1 to come up
   [00:00:07.595,000] <inf> phy_mii: PHY (7) Link speed 1000 Mb, full duplex

   [00:00:07.595,000] <inf> nxp_s32_netc_sample: Configuring iface 1 (0x318008f0)
   [00:00:07.595,000] <inf> nxp_s32_netc_sample: IPv6 address: 2001:db8::1
   [00:00:07.595,000] <inf> nxp_s32_netc_sample: IPv4 address: 192.0.2.1

To build and run the sample application for use-case 2:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp_s32/netc
   :board: s32z2xxdc2/s32z270/rtu0
   :goals: build flash
   :gen-args: -DDTC_OVERLAY_FILE="./vsi-and-psi.overlay"

Once started, you should see the network interfaces details, for example:

.. code-block:: console

   [00:00:00.051,000] <inf> phy_mii: PHY (7) ID 1CC916

   [00:00:00.052,000] <inf> nxp_s32_eth_psi: SI0 MAC: 00:00:00:01:02:00
   [00:00:00.052,000] <inf> nxp_s32_eth_vsi: SI1 MAC: 00:00:00:01:02:11
   [00:00:00.058,000] <inf> shell_telnet: Telnet shell backend initialized
   [00:00:00.058,000] <inf> nxp_s32_netc_sample: Starting sample
   [00:00:00.058,000] <inf> nxp_s32_netc_sample: Waiting for iface 1 to come up
   [00:00:07.595,000] <inf> phy_mii: PHY (7) Link speed 1000 Mb, full duplex

   [00:00:07.595,000] <inf> nxp_s32_netc_sample: Configuring iface 1 (0x3182f31c)
   [00:00:07.595,000] <inf> nxp_s32_netc_sample: IPv6 address: 2001:db8::1
   [00:00:07.595,000] <inf> nxp_s32_netc_sample: IPv4 address: 192.0.2.1
   [00:00:07.595,000] <inf> nxp_s32_netc_sample: Configuring iface 2 (0x3182f328)
   [00:00:07.595,000] <inf> nxp_s32_netc_sample: IPv6 address: 2001:db8::2
   [00:00:07.595,000] <inf> nxp_s32_netc_sample: IPv4 address: 192.0.2.2

Setting up Host
***************

To be able to reach the board from the host, it's needed to configure the host
network interface IP's and default routes. This guide assumes the host IPv4 and
IPv6 addresses are `192.0.2.3` and `2001:db8::3`, respectively. For example,
using a network interface named `enp1s0` in a GNU/Linux host or `Ethernet` in
a Windows host, this can be done with the following commands:

.. tabs::

   .. group-tab:: Linux

      .. code-block:: console

         ip -4 addr add 192.0.2.3/24 dev enp1s0
         ip -6 addr add 2001:db8::3/128 dev enp1s0
         route -A inet6 add default dev enp1s0

   .. group-tab:: Windows

      .. code-block:: console

         netsh interface ipv4 set address "Ethernet" static 192.0.2.3 255.255.255.0
         netsh interface ipv6 add address "Ethernet" 2001:db8::3/128
         netsh interface ipv6 add route ::/0 "Ethernet" ::

.. note::
   The above commands must be run as priviledged user.

If everything is configured correctly, you will be able to successfully execute
the following commands from the Zephyr shell:

.. code-block:: console

   net ping -I<iface> 192.0.2.3
   net ping -I<iface> 2001:db8::3

Where `<iface>` is the interface number starting from 1.
