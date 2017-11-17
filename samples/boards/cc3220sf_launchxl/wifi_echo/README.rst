.. _wifi-echo-sample:

Socket Echo Sample over CC3220SF WiFi
#####################################

Overview
********

This sample reimplements the networking sockets echo sample, leveraging
the CC3220SF WiFi host driver, which is part of the SimpleLink SDK.

This is based on the :ref:`sockets-echo-sample`,
with the following differences:

* Since SimpleLink's ``arpa/inet.h`` is missing inet_ntop, rather than
  pulling in the Zephyr networking stack just for an IP printing
  utility function, the sample code calling inet_ntop is removed;
* Zephyr's printk is used, showing how applications can still
  use Zephyr libraries (and drivers), and directly access the WiFi
  subsystem offered by SimpleLink.

Building and Running
********************

Building
========

The WiFi SSID and password are passed via environment variables.
This is to avoid having passwords in Kconfig files being accidentally
pushed into online repositories.

To build:

.. code-block:: console

  % mkdir build; cd build
  % export CFLAGS="-DNET_SSID="<yourSSID>" -DNET_PASS="<yourPassword>""
  % cmake .. -DBOARD=cc3220sf_launchxl
  % make

Running
========

When the sample starts up on the board, it will print via the serial
port the IP address assigned via DHCP to the board: <board_ip>

To validate the echo server, from a linux PC, run netcat:

.. code-block:: console

  % nc <board_ip> 4242

then, type lines into netcat which should be echoed back from the
CC3220SF board.

