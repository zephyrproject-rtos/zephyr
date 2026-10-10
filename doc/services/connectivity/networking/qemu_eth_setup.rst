.. _networking_with_eth_qemu:

Networking with QEMU Ethernet
#############################

.. contents::
    :local:
    :depth: 2

This page describes how to set up a virtual network between a (Linux) host
and a Zephyr application running in QEMU.

In this example, the :zephyr:code-sample:`sockets-echo-server` sample application from
the Zephyr source distribution is run in QEMU. The Zephyr instance is
connected to a Linux host using a tuntap device which is modeled in Linux as
an Ethernet network interface.

Prerequisites
*************

On the Linux Host, find the Zephyr `net-tools`_ project, which can either be
found in a Zephyr standard installation under the ``tools/net-tools`` directory
or installed stand alone from its own git repository:

.. code-block:: console

   git clone https://github.com/zephyrproject-rtos/net-tools


Basic Setup
***********

For the steps below, you will need two terminal windows:

* Terminal #1 is terminal window with net-tools being the current
  directory (``cd net-tools``)
* Terminal #2 is your usual Zephyr development terminal,
  with the Zephyr environment initialized.

When configuring the Zephyr instance, you must select the correct Ethernet
driver for QEMU connectivity:

* For ``qemu_x86``, select ``Intel(R) PRO/1000 Gigabit Ethernet driver``
  Ethernet driver. Driver is called ``e1000`` in Zephyr source tree.
* For ``qemu_cortex_m3``, select ``TI Stellaris MCU family ethernet driver``
  Ethernet driver. Driver is called ``stellaris`` in Zephyr source tree.
* For ``mps2_an385``, select ``SMSC911x/9220 Ethernet driver`` Ethernet driver.
  Driver is called ``smsc911x`` in Zephyr source tree.
* For ``qemu_cortex_a53``, ``Intel(R) PRO/1000 Gigabit Ethernet driver``
  Ethernet driver is selected by default.
* Additionally, the :zephyr:code-sample:`sockets-echo-server` sample contains
  overlay files for the VIRTIO Network device on ``qemu_x86_64``.

Step 1 - Create Ethernet interface
==================================

Before starting QEMU with network connectivity, a network interface
should be created in the host system.

In terminal #1, type:

.. code-block:: console

   ./net-setup.sh

You can tweak the behavior of the ``net-setup.sh`` script. See various options
by running ``net-setup.sh`` like this:

.. code-block:: console

   ./net-setup.sh --help


Step 2 - Start app in QEMU board
================================

Build and start the :zephyr:code-sample:`sockets-echo-server` sample application.
In this example, the qemu_x86 board is used.

In terminal #2, type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: qemu_x86
   :gen-args: -DEXTRA_CONF_FILE=overlay-e1000.conf
   :goals: run
   :compact:

Alternatively, if you decided to use the VIRTIO Network device on qemu_x86_64:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: qemu_x86_64
   :gen-args: -DDTC_OVERLAY_FILE=virtnet.overlay -DEXTRA_CONF_FILE=overlay-virtnet.conf
   :goals: run
   :compact:

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

macOS Host
**********

macOS has no tuntap device, so on a macOS host the build system connects QEMU
to the host through the vmnet framework in host mode instead of a TAP
interface. No interface has to be created beforehand and
:kconfig:option:`CONFIG_ETH_QEMU_IFACE_NAME` is not used: vmnet creates a
bridge interface on the host when QEMU starts and removes it when QEMU exits.
The instance joins an isolated vmnet network without a DHCP server, so the
host side of the bridge starts without an address and the ``192.0.2.0/24``
and ``2001:db8::/64`` addresses used by the samples can be assigned to it.

The network is identified by a UUID that the build system generates for each
build directory and keeps in its CMake cache, so instances started from
different build directories are on separate networks with separate bridges.
To put several builds on one network, pass the same identifier to each of
them with ``-DNET_QEMU_VMNET_NET_UUID=<uuid>``.

Opening a vmnet interface requires root privileges unless the QEMU binary
carries the ``com.apple.vm.networking`` entitlement. The QEMU builds shipped
with the Zephyr SDK do not have it, so QEMU must be started as root.
Build the application as a normal user, then start the ``run`` target with
``sudo``:

.. code-block:: console

   west build -b qemu_x86 samples/net/sockets/echo_server -- \
      -DEXTRA_CONF_FILE=overlay-e1000.conf
   sudo ninja -C build run

Once QEMU runs, assign the host addresses to the bridge interface vmnet
created in a second terminal. The interface is ``bridge100`` unless other
virtual machines are running; ``ifconfig`` shows it with a ``vmenet``
interface as member:

.. code-block:: console

   sudo ifconfig bridge100 alias 192.0.2.2 255.255.255.0
   sudo ifconfig bridge100 inet6 2001:db8::2 prefixlen 64 alias

The host now reaches the Zephyr instance at ``192.0.2.1`` and ``2001:db8::1``.
The addresses have to be assigned again after every QEMU start, since the
bridge is recreated each time.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
