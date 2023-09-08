.. zephyr:code-sample:: so_txtime
   :name: UDP sender using SO_TXTIME
   :relevant-api: bsd_sockets ethernet

   Control the transmission time of a packet using SO_TXTIME socket option.

Overview
********

This sample is a simple UDP sender/receiver which will set the
SO_TXTIME socket option and expects the Ethernet driver to send
the data when the TX time is expected. The application requires
that the board has PTP clock support. A simulated PTP clock is
provided for qemu_x86 board. Also frdm_k64f and sam_e70_xplained boards
are supported. Other mcux or gmac Ethernet driver based boards should
work too.
User can control how long the application should wait between packets sent by
setting :kconfig:option:`CONFIG_NET_SAMPLE_PACKET_INTERVAL` option.
Also the TXTIME value can be specified in the config file by setting the
:kconfig:option:`CONFIG_NET_SAMPLE_PACKET_TXTIME` option. In this case the value is
used as an offset from the current time.

Building and Running
********************

When the application is run, it starts to send UDP packets. You can start
``echo-server`` application from `net-tools`_ project to catch these and
send the data back to this application. Optionally you can set
:kconfig:option:`CONFIG_NET_SAMPLE_PACKET_SOCKET` option, which makes the application
to create an ``AF_PACKET`` type socket. In this case, the ``echo-server``
application cannot be used as a peer.

This sample can be built and executed on qemu_x86 board as
described in :ref:`networking_with_host`.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
