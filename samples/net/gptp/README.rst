.. zephyr:code-sample:: gptp
   :name: gPTP
   :relevant-api: gptp ptp_time

   Enable gPTP support and monitor functionality using net-shell.

Overview
********

The gPTP sample application for Zephyr will enable gPTP support, registers
gPTP phase discontinuity callback, enable traffic class support (TX multi
queues) and setup VLANs (if enabled). The net-shell is also enabled so that
user can monitor gPTP functionality.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/gptp`.

Requirements
************

For generic host connectivity, that can be used for debugging purposes, see
:ref:`networking_with_native_sim` for details.

Building and Running
********************

A good way to run this sample is to run this gPTP application inside
native_sim board as described in :ref:`networking_with_native_sim` or with
embedded device like NXP FRDM-K64F, Nucleo-H743-ZI, Nucleo-H745ZI-Q,
Nucleo-F767ZI or Atmel SAM-E70 Xplained. Note that gPTP is only supported for
boards that have an Ethernet port and which has support for collecting
timestamps for sent and received Ethernet frames.

Follow these steps to build the gPTP sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/gptp
   :board: <board to use>
   :goals: build
   :compact:

The net-shell command "**net gptp**" will print out general gPTP information.
For port 1, the command "**net gptp 1**" will print detailed information about
port 1 statistics etc. Note that executing the shell command could affect
the timing of the sent or received gPTP packets and the grandmaster might
mark the device as non AS capable and disable it.

Setting up Linux Host
=====================

If you need VLAN support in your network, then the
:zephyr_file:`samples/net/vlan/vlan-setup-linux.sh` provides a script that can be
executed on the Linux host. It creates two VLANs on the Linux host and creates
routes to Zephyr. If you are using native_sim board, then
the ``net-setup.sh`` will create VLAN setup automatically with this command:

.. code-block:: console

   ./net-setup.sh -c zeth-vlan.conf

The OpenAVNU repository at https://github.com/AVnu contains gPTP
daemon that can be run in Linux host and which can act as a grandmaster for
the IEEE 801.1AS network. Note that OpenAVNU will not work with
native_sim board as that board only supports software timestamping and
OpenAVNU only supports hardware timestamping. See instructions at the end
of this chapter how to run linuxptp daemon with native_sim board.

Get OpenAvnu/gPTP project sources

.. code-block:: console

    git clone git@github.com:AVnu/gptp.git

After downloading the source code, compile it like this in Linux:

.. code-block:: console

    mkdir build
    cd build
    cmake ..
    make
    cp ../gptp_cfg.ini .

Edit the :file:`gptp_cfg.ini` file and set the neighborPropDelayThresh to 10000
as the default value 800 is too low if you run the gPTP in FRDM-K64F.

Then execute the daemon with correct network interface and the configuration
file.

.. code-block:: console

    sudo ./gptp enp0s25 -F gptp_cfg.ini

Note that here the example network interface **enp0s25** is the name of the
network interface that is connected to your Zephyr device.

If everything is configured correctly, you should see following kind of
messages from gptp:

.. code-block:: console

    INFO     : GPTP [13:01:14:837] gPTP starting
    INFO     : GPTP [13:01:14:838] priority1 = 248
    INFO     : GPTP [13:01:14:838] announceReceiptTimeout: 3
    INFO     : GPTP [13:01:14:838] syncReceiptTimeout: 3
    INFO     : GPTP [13:01:14:838] LINKSPEED_100MB - PHY delay
			TX: 1044 | RX: 2133
    INFO     : GPTP [13:01:14:838] LINKSPEED_1G - PHY delay
			TX: 184 | RX: 382
    INFO     : GPTP [13:01:14:838] neighborPropDelayThresh: 10000
    INFO     : GPTP [13:01:14:838] syncReceiptThreshold: 8
    ERROR    : GPTP [13:01:14:838] Using clock device: /dev/ptp0
    STATUS   : GPTP [13:01:14:838] Starting PDelay
    STATUS   : GPTP [13:01:14:838] Link Speed: 1000000 kb/sec
    STATUS   : GPTP [13:01:14:871] AsCapable: Enabled
    STATUS   : GPTP [13:01:16:497] New Grandmaster "3C:97:0E:FF:FE:23:F2:32" (previous "00:00:00:00:00:00:00:00")
    STATUS   : GPTP [13:01:16:497] Switching to Master

If Zephyr syncs properly with gptp daemon, then this is printed:

.. code-block:: console

    STATUS   : GPTP [13:01:25:965] AsCapable: Enabled

By default gPTP in Zephyr will not print any gPTP debug messages to console.
One can enable debug prints by setting
:kconfig:option:`CONFIG_NET_GPTP_LOG_LEVEL_DBG` in the config file.

For native_sim board, use ``linuxptp`` project as that supports
software timestamping.

Get linuxptp project sources

.. code-block:: console

    git clone git://git.code.sf.net/p/linuxptp/code

Compile the ``ptp4l`` daemon and start it like this:

.. code-block:: console

    sudo ./ptp4l -2 -f gPTP-zephyr.cfg -i zeth -m -q -l 6 -S

Use the ``default.cfg`` as a base, copy it to ``gPTP-zephyr.cfg``, and modify
it according to your needs.


Multiport Setup
===============

If you set :kconfig:option:`CONFIG_NET_GPTP_NUM_PORTS` larger than 1, then gPTP sample
will create multiple TSN ports. This configuration is currently only supported
in native_sim board.

You need to enable the ports in the net-tools. If the number of ports is set
to 2, then give following commands to create the network interfaces in host
side:

.. code-block:: console

    sudo ./net-setup.sh -c zeth0-gptp.conf -i zeth0 start
    sudo ./net-setup.sh -c zeth1-gptp.conf -i zeth1 start

After that you can start ptp4l daemon for both interfaces. Please use two
terminals when starting ptp4l daemon. Note that you must use ptp4l as OpenAVNU
does not work with software clock available in native_sim.

.. code-block:: console

    cd <ptp4l directory>
    sudo ./ptp4l -2 -f gPTP-zephyr.cfg -m -q -l 6 -S -i zeth0
    sudo ./ptp4l -2 -f gPTP-zephyr.cfg -m -q -l 6 -S -i zeth1

Compile Zephyr application.

.. zephyr-app-commands::
   :zephyr-app: samples/net/gptp
   :board: native_sim
   :goals: build
   :compact:

When the Zephyr image is build, you can start it like this:

.. code-block:: console

    build/zephyr/zephyr.exe -attach_uart
