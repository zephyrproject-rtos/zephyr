.. _slip_throughput_server:

COBS Throughput Server Sample
##############################

Overview
********

This sample implements a UDP echo server for testing SLIP UART async interface
throughput. The server receives UDP packets on port 5001, verifies the data
pattern, and echoes them back to the client.

The sample is designed to work with the COBS Throughput Client sample running
on another board connected via a serial link.

Requirements
************

* A board with UART support and async API
* A serial connection to another board running the client sample
* Network stack with UDP socket support

Building and Running
********************

Build the sample for your board:

.. code-block:: console

   west build -b <your_board> samples/net/cobs_throughput_server

Flash to your board:

.. code-block:: console

   west flash

Configuration
*************

The server requires a devicetree overlay to configure the COBS UART interface.
Example overlay (boards/<your_board>.overlay):

.. code-block:: devicetree

   / {
       #size-cells = <0>;

       cobs: cobs@0 {
           compatible = "zephyr,cobs-uart-async";
           uart = <&uart0>;
           status = "okay";
       };
   };

The server automatically configures the COBS interface with:
- IPv4: 192.168.7.1/24
- IPv6: fe80::1

Usage
*****

1. Connect two boards via their UART interfaces (TX <-> RX, GND <-> GND)
2. Flash this server sample on board 1
3. Flash the cobs_throughput_client sample on board 2
4. Start the server (it will wait for connections)
5. On the client board, use the shell to start the test:

   .. code-block:: console

      uart:~$ throughput start

The server will display statistics every 10 seconds showing throughput,
packet counts, and error rates.

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   [00:00:00.000,000] <inf> cobs_server: COBS Throughput Server
   [00:00:00.100,000] <inf> cobs_server: Using COBS interface: 1
   [00:00:00.200,000] <inf> cobs_server: COBS interface is up
   [00:00:00.201,000] <inf> cobs_server: Server listening on port 5001
   [00:00:00.202,000] <inf> cobs_server: Max packet size: 1472 bytes
   [00:00:10.000,000] <inf> cobs_server: === Statistics (elapsed: 10 s) ===
   [00:00:10.001,000] <inf> cobs_server:   RX: 1000 pkts, 512000 bytes (409 kbps)
   [00:00:10.002,000] <inf> cobs_server:   TX: 1000 pkts, 512000 bytes (409 kbps)
   [00:00:10.003,000] <inf> cobs_server:   Errors: 0, Pattern errors: 0

