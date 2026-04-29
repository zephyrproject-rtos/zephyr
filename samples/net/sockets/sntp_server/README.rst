.. zephyr:code-sample:: sntp-server
   :name: SNTP server
   :relevant-api: bsd_sockets sntp

   SNTP service for setting the current time.
   The system will act as a server for clients on the network.

Overview
********

This sample is a simple SNTP server implementation.
The demo will query a SNTP server and set the system time and date.
When the local clock is set, the system will respond to SNTP queries
on port 123, the default SNTP port.
You can query the server using a SNTP client, such as ntpdig or sntp.

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
