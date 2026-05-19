.. _bluetooth_spp_uart_client_sample:

Bluetooth: Classic: SPP UART Client
####################################

Overview
********

This sample demonstrates SPP UART in **client mode**. The device initiates
a BR/EDR connection to a remote SPP server, automatically discovers the
SPP service via SDP, and opens an RFCOMM channel. Data is then exchanged
through the standard Zephyr UART API.

The SPP UART driver handles the entire connection sequence automatically:

1. Application calls ``bt_conn_create_br()`` to establish ACL
2. Driver detects BR/EDR connection via ``bt_conn_cb``
3. Driver performs SDP discovery to find RFCOMM channel
4. Driver connects RFCOMM DLC
5. UART becomes ready for data transfer

Requirements
************

* A board with Bluetooth Classic (BR/EDR) support
* A remote device running an SPP server (e.g., the ``spp_uart`` server sample)

Building and Running
********************

Edit ``src/main.c`` and set ``REMOTE_ADDR`` to your SPP server's address:

.. code-block:: c

   #define REMOTE_ADDR "AA:BB:CC:DD:EE:FF"

Then build:

.. code-block:: console

   west build -b <board> samples/bluetooth/classic/spp_uart_client

Testing with Server Sample
***************************

Terminal 1 (server):

.. code-block:: console

   west build -b native_sim/native/64 samples/bluetooth/classic/spp_uart_server
   west build -t run

Terminal 2 (client):

.. code-block:: console

   west build -b native_sim/native/64 samples/bluetooth/classic/spp_uart_client
   west build -t run
