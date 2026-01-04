.. _cobs_uart_async:

COBS UART async network interface
################################

Overview
********

The COBS UART async driver lets you run Zephyr IP networking over a simple UART
link (TX/RX + GND) that uses COBS framing.

It uses:

- the UART Async API for efficient transfers, and
- COBS framing to turn the UART byte stream into packet-sized frames.

This is a good fit for point-to-point links like:

- Device-to-device communication
- Gateway connections
- Debug / bring-up networking over a spare UART

The driver supports multiple instances: you can create multiple COBS network
interfaces by binding multiple devicetree nodes to different UART peripherals.

Enabling the driver
******************

Kconfig
=======

Enable networking and the COBS UART async driver:

.. code-block:: kconfig

   CONFIG_NETWORKING=y
   CONFIG_NET_DRIVERS=y
   CONFIG_NET_NATIVE=y
   CONFIG_COBS_UART_ASYNC=y
   CONFIG_NET_L2_COBS_SERIAL=y
   CONFIG_COBS=y
   CONFIG_UART_ASYNC_API=y

Then enable whichever IP stack you want to use (IPv4/IPv6) and any application
protocols (UDP/TCP/sockets). For example:

.. code-block:: kconfig

   CONFIG_NET_IPV6=y
   CONFIG_NET_UDP=y
   CONFIG_NET_SOCKETS=y

Optional tuning options (use defaults unless you have a reason to change them):

- ``CONFIG_COBS_UART_ASYNC_MTU`` - MTU (default: 1500)
- ``CONFIG_COBS_UART_ASYNC_RX_BUF_LEN`` - UART RX buffer size (default: 128)
- ``CONFIG_COBS_UART_ASYNC_TX_BUF_LEN`` - UART TX buffer size (default: 2048)
- ``CONFIG_COBS_UART_ASYNC_RINGBUF_SIZE`` - Ring buffer for Rx data (default: 512)
- ``CONFIG_COBS_UART_ASYNC_RX_PRIORITY`` - Rx worker priority (default: 7)
- ``CONFIG_COBS_UART_ASYNC_INIT_PRIORITY`` - init priority (should be higher
  than the UART init priority on your system)

Devicetree
==========

Create one devicetree node per network interface and point it at the UART to
use via the ``uart`` phandle.

.. code-block:: devicetree

   / {
     #address-cells = <1>;
     #size-cells = <0>;

     cobs0: cobs@0 {
       compatible = "zephyr,cobs-uart-async";
       reg = <0>;
       uart = <&uart1>;
       status = "okay";
     };
   };

   &uart1 {
     status = "okay";
     current-speed = <115200>;
   };

You can add a second interface (for example on a board with two UARTs) by adding
``cobs1: cobs@1 { ... uart = <&uart2>; };``.

Wiring
******

The minimal link connection needed is a plain UART connection:

- Connect **TX** of one side to **RX** of the other
- Connect **RX** of one side to **TX** of the other
- Connect **GND** between the boards

Bringing the interface up
*************************

Once enabled, the driver creates a network interface automatically. You can
find it by its L2 type and then bring it up like any other Zephyr interface:

.. code-block:: c

   #include <zephyr/net/net_if.h>
   #include <zephyr/net/net_core.h>
   #include <zephyr/net/net_l2.h>
   #include <zephyr/net/net_ip.h>

   /* Get the first COBS serial interface (use other discovery if you have
    * multiple instances).
    */
   struct net_if *iface =
	   net_if_get_first_by_type(&NET_L2_GET_NAME(COBS_SERIAL_L2));

   net_if_up(iface);

   /* Assign an address (example: IPv6 ULA) */
   struct in6_addr addr6;
   net_addr_pton(AF_INET6, "fd00:1::1", &addr6);
   net_if_ipv6_addr_add(iface, &addr6, NET_ADDR_MANUAL, 0);

After that you can use the regular Zephyr networking APIs (including BSD
sockets) on this interface.

Notes for multi-interface setups
********************************

If you define multiple ``zephyr,cobs-uart-async`` nodes, you will get multiple
interfaces. In that case, prefer selecting interfaces by name or by device
instance (for example using ``CONFIG_NET_INTERFACE_NAME`` and
``net_if_get_by_name()``), rather than assuming “the first one”.

Tests
*****

- The sockets test (:ref:`cobs_uart_async_sockets_test`) demonstrates bringing
  up two interfaces and sending UDP traffic between them.

