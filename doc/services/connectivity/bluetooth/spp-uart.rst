.. _bluetooth_spp_uart:

Bluetooth Classic SPP UART
##########################

Overview
********

The SPP UART driver provides a virtual UART interface over Bluetooth Classic
Serial Port Profile (SPP/RFCOMM). It follows the same design pattern as the
NUS UART driver (``CONFIG_BT_ZEPHYR_NUS``) but operates over BR/EDR connections instead
of BLE.

Once configured, applications can use the standard Zephyr UART API
(``uart_poll_in``, ``uart_poll_out``, ``uart_fifo_fill``, ``uart_fifo_read``)
to send and receive data over an RFCOMM channel. This also enables using SPP
as a console or shell backend without any application code changes.

Configuration
*************

Enable the driver with:

.. code-block:: kconfig

   CONFIG_BT=y
   CONFIG_BT_CLASSIC=y
   CONFIG_BT_RFCOMM=y
   CONFIG_UART_BT_SPP=y

Devicetree
**********

Add a node with compatible ``zephyr,bt-spp-uart``:

.. code-block:: devicetree

   / {
       bt_spp_uart: bt_spp_uart {
           compatible = "zephyr,bt-spp-uart";
           role = "server";
           channel = <0>;        /* 0 = auto-allocate */
           tx-fifo-size = <1024>;
           rx-fifo-size = <1024>;
       };
   };

Properties:

- ``role``: ``"server"`` (default) or ``"client"``
- ``channel``: RFCOMM channel number.

  - Server mode: ``0`` = auto-allocate a local server channel
  - Client mode: ``0`` = discover the remote server channel via SDP

- ``tx-fifo-size``: TX ring buffer size in bytes
- ``rx-fifo-size``: RX ring buffer size in bytes

Console and Shell
*****************

To redirect console and shell over SPP, use the built-in snippet:

.. code-block:: console

   west build -S spp-console [...]

This sets ``zephyr,console`` and ``zephyr,shell-uart`` to the SPP UART device.
A remote device (phone, PC) can connect via SPP and interact with the Zephyr
shell.

Server vs Client Mode
*********************

**Server mode** (default): The device registers an RFCOMM server channel and
advertises it via SDP. A remote device discovers and connects to it.

**Client mode**: The device initiates an RFCOMM connection to a remote SPP
server after a BR/EDR ACL link is established.
