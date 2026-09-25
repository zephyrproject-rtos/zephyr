.. zephyr:code-sample:: sntp-server
   :name: SNTP server
   :relevant-api: bsd_sockets sntp sntp_server

   Serve the current time to SNTP clients on the network.

Overview
********

This sample is a simple SNTP server implementation.

At startup the sample queries the SNTP server configured in
:kconfig:option:`CONFIG_NET_SAMPLE_SNTP_UPSTREAM_ADDRESS` and sets the system
clock from the reply. It then serves time to clients on UDP port 123, the
default SNTP port, on every enabled address family.

The upstream server has no default, set it according to your network setup,
for example ``pool.ntp.org``. Without it the sample still starts the server,
but the server tells its clients that its clock is not synchronized and the
timestamps must not be used.

You can query the server with an SNTP client, such as ``ntpdig`` or ``sntp``.

This demo assumes that the platform of choice has networking support,
some adjustments to the configuration may be needed.

Building and Running
********************

See the `net-tools`_ project for more details.

This sample can be built and executed on QEMU or native_sim board as
described in :ref:`networking_with_qemu`.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools

Wi-Fi
=====

The IPv4 Wi-Fi support can be enabled in the sample with
:ref:`Wi-Fi snippet <snippet-wifi-ipv4>`.
