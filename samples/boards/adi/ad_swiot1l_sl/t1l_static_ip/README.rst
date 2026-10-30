.. zephyr:code-sample:: ad_swiot1l_sl_t1l_static_ip
   :name: AD-SWIOT1L-SL 10BASE-T1L static IPv4
   :relevant-api: net_if net_mgmt

   Bring up the AD-SWIOT1L-SL 10BASE-T1L interface with a static IPv4 address.

Overview
********

This sample brings up the ADIN1110 10BASE-T1L interface of the AD-SWIOT1L-SL
with the static IPv4 address ``192.168.97.40/16``. The green LED follows the
T1L link state and the network shell is available on the console.

Requirements
************

- AD-SWIOT1L-SL board, powered from its 24 V terminal block.
- A 10BASE-T1L link partner, for example a USB to 10BASE-T1L media converter,
  configured with an address in ``192.168.0.0/16``.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/adi/ad_swiot1l_sl/t1l_static_ip
   :board: ad_swiot1l_sl
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   <inf> phy_adin: PHY 1 ID 283BC91
   *** Booting Zephyr OS build v4.5.0 ***
   <inf> net_config: IPv4 address: 192.168.97.40
   <inf> t1l_static_ip: IPv4 192.168.97.40/255.255.0.0
   <inf> t1l_static_ip: T1L link up

The board then answers ping from the link partner, and ``net iface``,
``net ping`` and ``net arp`` can be used from the shell.
