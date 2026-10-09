.. _bluetooth_spp_uart:

Bluetooth Classic SPP UART
##########################

Overview
********

The SPP UART driver provides a virtual UART interface over Bluetooth Classic
Serial Port Profile (SPP/RFCOMM). It follows the same design pattern as the
NUS UART driver (``CONFIG_UART_BT``, which builds on the NUS GATT service
``CONFIG_BT_ZEPHYR_NUS``) but operates over BR/EDR connections instead
of BLE.

Once configured, applications can use the standard Zephyr UART API
(``uart_poll_in``, ``uart_poll_out``, ``uart_fifo_fill``, ``uart_fifo_read``)
to send and receive data over an RFCOMM channel. This also enables using SPP
as a console or shell backend without any application code changes.

Each devicetree instance acts as an SPP server: it registers an RFCOMM server
channel, advertises it via SDP and accepts one incoming connection at a time.

Configuration
*************

Enable the driver with:

.. code-block:: kconfig

   CONFIG_BT=y
   CONFIG_BT_CLASSIC=y
   CONFIG_BT_RFCOMM=y
   CONFIG_UART_BT_SPP=y

:kconfig:option:`CONFIG_UART_BT_SPP_AUTO_START_BLUETOOTH` enables Bluetooth and
makes the device BR/EDR connectable and discoverable at boot, for applications
that do not use Bluetooth themselves. Applications that enable Bluetooth
themselves must leave it disabled.

Devicetree
**********

Add a node with compatible ``zephyr,bt-spp-uart``:

.. code-block:: devicetree

   / {
       bt_spp_uart: bt_spp_uart {
           compatible = "zephyr,bt-spp-uart";
           channel = <0>;        /* 0 = auto-allocate */
           tx-fifo-size = <1024>;
       };
   };

Properties:

- ``channel``: local RFCOMM server channel (1-30), advertised via SDP.
  ``0`` = auto-allocate a dynamic channel.

- ``tx-fifo-size``: TX ring buffer size in bytes. The RX path has no byte
  FIFO: received RFCOMM frames are held as-is and their per-frame credits
  provide native flow control.

Console and Shell
*****************

To redirect console and shell over SPP, use the built-in snippet:

.. code-block:: console

   west build -S spp-console [...]

This sets ``zephyr,console`` and ``zephyr,shell-uart`` to the SPP UART device
and enables :kconfig:option:`CONFIG_UART_BT_SPP_AUTO_START_BLUETOOTH`. A remote
device (phone, PC) can connect via SPP and interact with the Zephyr shell.

Data Flow
*********

Received RFCOMM frames are held until the application has read them, and the
RX credit of each frame is returned only then, so a slow reader throttles the
remote device instead of losing data.

Written bytes go to a TX ring buffer of ``tx-fifo-size`` bytes and are sent in
frames of up to the RFCOMM MTU from the driver's work queue. Up to
:kconfig:option:`CONFIG_UART_BT_SPP_TX_BUF_COUNT` frames per instance can be
waiting for TX credits from the remote device. When the ring buffer is full,
``uart_fifo_fill()`` returns a short count, and ``uart_poll_out()`` waits for
space when called from a thread, or drops the byte when called from an ISR, from
the driver's work queue (e.g. an IRQ callback) or while not connected.
