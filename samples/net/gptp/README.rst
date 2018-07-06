.. _gptp-sample:

gPTP Sample Application
#######################

Overview
********

The gPTP sample application for Zephyr will enable gPTP support, registers
gPTP phase discontinuity callback, enable traffic class support (TX multi
queues) and setup VLANs (if enabled). The net-shell is also enabled so that
user can monitor gPTP functionality.

The source code for this sample application can be found at:
:file:`samples/net/gptp`.

Requirements
************

- :ref:`networking_with_qemu`

Building and Running
********************

A good way to run this sample is to run this gPTP application inside QEMU
as described in :ref:`networking_with_qemu` or with embedded device like
FRDM-K64F. Note that gPTP is only supported for boards that have ethernet port
and which has support for collecting timestamps for sent and received
ethernet frames.

Follow these steps to build the gPTP sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/gptp
   :board: <board to use>
   :conf: prj.conf
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
:file:`samples/net/vlan/vlan-setup-linux.sh` provides a script that can be
executed on the Linux host. It creates two VLANs on the Linux host and creates
routes to Zephyr.

The OpenAVNU repository at https://github.com/AVnu/OpenAvnu contains gPTP
daemon that can be run in Linux host and which can act as a grandmaster for
the IEEE 801.1AS network.

After downloading the source code, compile it like this in Linux:

.. code-block:: console

    mkdir build
    cd build
    cmake ..
    make
    cp daemons/gptp/gptp_cfg.ini build/daemons/gptp/
    cd build/daemons/gptp

Edit the :file:`gptp_cfg.ini` file and set the neighborPropDelayThresh to 10000
as the default value 800 is too low if you run the gPTP in FRDM-K64F.

Then execute the daemon with correct network interface and the configuration
file.

.. code-block:: console

    sudo ./gptp enp0s25 -F gptp_cfg.ini

Note that here the example network interface **enp0s25** is the name of the
non-VLAN network interface that is connected to your Zephyr device.

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
One can enable debug prints by setting :option:`CONFIG_NET_DEBUG_GPTP` in
the config file.
