.. _bluetooth_spp_uart_server_sample:

Bluetooth: Classic: SPP UART Server
####################################

Overview
********

This sample demonstrates how to use the SPP UART driver
(``CONFIG_UART_BT_SPP``) to exchange serial data over Bluetooth Classic
RFCOMM. The device acts as an SPP server and echoes received data back
to the remote client.

The SPP UART driver exposes a standard Zephyr UART device, so any
application that works with UART (shell, console, logging, custom
protocols) can transparently run over Bluetooth Classic without code
changes.

Requirements
************

* A board with Bluetooth Classic (BR/EDR) support
* A remote device with an SPP client (phone app, PC terminal, etc.)

Building and Running
********************

.. code-block:: console

   west build -b <board> samples/bluetooth/classic/spp_uart_server

For boards without native BT Classic support, you can test with
``native_sim`` and a USB BT dongle:

.. code-block:: console

   west build -b native_sim/native/64 samples/bluetooth/classic/spp_uart_server

Usage
*****

1. Flash and run the sample
2. The device becomes discoverable as "Zephyr SPP"
3. Connect from a phone/PC SPP client
4. Any data sent to the device is echoed back
5. Every 10 seconds, a heartbeat message is sent to the remote

Using as Console/Shell
**********************

To redirect the Zephyr shell over SPP, use the ``spp-console`` snippet:

.. code-block:: console

   west build -b <board> samples/subsys/shell/shell_module -S spp-console

This gives you a full interactive shell over Bluetooth Classic SPP.
