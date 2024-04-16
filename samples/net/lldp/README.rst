.. zephyr:code-sample:: lldp
   :name: Link Layer Discovery Protocol (LLDP)
   :relevant-api: lldp net_l2

   Enable LLDP support and setup VLANs.

Overview
********

The Link Layer Discovery Protocol sample application for Zephyr will enable
LLDP support and setup VLANs if needed.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/lldp`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

A good way to run this sample LLDP application is inside QEMU,
as described in :ref:`networking_with_qemu` or with embedded device like
FRDM-K64F. Note that LLDP is only supported for boards that have an ethernet
port.

Follow these steps to build the LLDP sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lldp
   :board: <board to use>
   :conf: prj.conf
   :goals: build
   :compact:

Setting up Linux Host
=====================

If you need VLAN support in your network, then the
:zephyr_file:`samples/net/vlan/vlan-setup-linux.sh` provides a script that can be
executed on the Linux host. It creates two VLANs on the Linux host and creates
routes to Zephyr.
