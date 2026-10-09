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

:kconfig:option:`CONFIG_UART_BT_SPP` is enabled by default when Bluetooth Classic
is enabled and the devicetree has an enabled ``zephyr,bt-spp-uart`` node:

.. code-block:: kconfig

   CONFIG_BT=y
   CONFIG_BT_CLASSIC=y

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
  ``0`` (the default) = auto-allocate a free dynamic channel.

- ``tx-fifo-size``: TX ring buffer size in bytes. The RX path has no byte
  FIFO: received RFCOMM frames are held as-is and their per-frame credits
  provide native flow control.

Data Flow
*********

Received RFCOMM frames are held until the application has read them, and the
RX credit of each frame is returned only then, so a slow reader throttles the
remote device instead of losing data.

Unread frames stay in the ACL RX buffers, which all connections and channels
share. Each instance can hold up to the RX credits RFCOMM gives its DLC, at most
:kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA`. With several instances
connected, slow readers can hold all buffers, and then no data arrives on other
channels either. Size :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA` for
the frames all instances can hold, plus the buffers the other users of the
links need.

Written bytes go to a TX ring buffer of ``tx-fifo-size`` bytes and are sent in
frames of up to the RFCOMM MTU from the driver's work queue. Up to
:kconfig:option:`CONFIG_UART_BT_SPP_TX_BUF_COUNT` frames per instance can be
waiting for TX credits from the remote device. When the ring buffer is full,
``uart_fifo_fill()`` returns a short count, and ``uart_poll_out()`` waits for
space when called from a thread. It drops the byte instead when the caller cannot
sleep (ISR, interrupts locked or a spinlock held), runs on the driver's work
queue (e.g. an IRQ callback) or while not connected. If no frame could be sent
for 100 ms, it also drops bytes until a frame is sent again, so that a caller on
the Bluetooth host's thread (e.g. ``printk()`` from a Bluetooth callback with
the SPP UART as console) does not block the credits it waits for.

Data written before the first connection (e.g. the boot banner) is kept in the
ring buffer and sent once a remote device connects. On disconnect, unsent data
is discarded, and data written until the next connection is dropped, so that
none of it reaches the next remote device, which may be another one.
