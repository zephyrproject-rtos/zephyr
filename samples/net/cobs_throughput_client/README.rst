.. _slip_throughput_client:

COBS Throughput Client Sample
##############################

Overview
********

This sample implements a configurable UDP throughput test client for the COBS
UART async interface. It sends UDP packets with a predictable pattern to a
server, receives the echoed responses, and measures throughput, latency, and
packet loss.

The client provides shell commands to configure test parameters before running.

Requirements
************

* A board with UART support and async API
* A serial connection to another board running the server sample
* Network stack with UDP socket support
* Shell support for interactive configuration

Building and Running
********************

Build the sample for your board:

.. code-block:: console

   west build -b <your_board> samples/net/cobs_throughput_client

Flash to your board:

.. code-block:: console

   west flash

Configuration
*************

The client requires a devicetree overlay to configure the COBS UART interface.
Example overlay (boards/<your_board>.overlay):

.. code-block:: devicetree

   / {
       #size-cells = <0>;

       cobs: cobs@0 {
           compatible = "zephyr,cobs-uart-async";
           uart = <&uart1>;
           status = "okay";
       };
   };

The client automatically configures the COBS interface with:
- IPv4: 192.168.7.2/24
- IPv6: fe80::2

Default test parameters (can be overridden via Kconfig or shell):
- Server IP: 192.168.7.1
- Packet size: 512 bytes
- Test duration: 60 seconds
- Delay between packets: 0 ms

Shell Commands
**************

The client provides interactive shell commands for test configuration:

.. code-block:: console

   throughput show                    - Show current configuration
   throughput set_size <bytes>        - Set packet size (16-1472 bytes)
   throughput set_server <ip>         - Set server IP address
   throughput set_duration <seconds>  - Set test duration (1-3600 seconds)
   throughput set_delay <ms>          - Set delay between packets (milliseconds)
   throughput start                   - Start the throughput test

Usage
*****

1. Connect two boards via their UART interfaces (TX <-> RX, GND <-> GND)
2. Flash the cobs_throughput_server sample on board 1
3. Flash this client sample on board 2
4. Configure test parameters using shell commands:

   .. code-block:: console

      uart:~$ throughput show
      uart:~$ throughput set_size 1024
      uart:~$ throughput set_duration 30
      uart:~$ throughput start

The test will run for the configured duration and display final statistics.

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   [00:00:00.000,000] <inf> cobs_client: COBS Throughput Client
   [00:00:00.100,000] <inf> cobs_client: Using COBS interface: 1
   [00:00:00.200,000] <inf> cobs_client: COBS interface is up
   
   uart:~$ throughput set_size 1024
   Packet size set to 1024 bytes
   uart:~$ throughput start
   Starting throughput test...
   [00:00:01.000,000] <inf> cobs_client: === Starting Throughput Test ===
   [00:00:01.001,000] <inf> cobs_client: Server: 192.168.7.1:5001
   [00:00:01.002,000] <inf> cobs_client: Packet size: 1024 bytes
   [00:00:01.003,000] <inf> cobs_client: Duration: 60 seconds
   [00:00:01.004,000] <inf> cobs_client: Test started...
   ...
   [00:01:01.000,000] <inf> cobs_client: === Final Statistics ===
   [00:01:01.001,000] <inf> cobs_client: Test duration: 60 seconds
   [00:01:01.002,000] <inf> cobs_client: Packet size: 1024 bytes
   [00:01:01.003,000] <inf> cobs_client: TX: 5000 pkts, 5120000 bytes (682 kbps)
   [00:01:01.004,000] <inf> cobs_client: RX: 4998 pkts, 5117952 bytes (682 kbps)
   [00:01:01.005,000] <inf> cobs_client: Packet loss: 2 (0.04%)
   [00:01:01.006,000] <inf> cobs_client: Errors: 0, Pattern errors: 0, Timeouts: 2
   [00:01:01.007,000] <inf> cobs_client: RTT (us): min=1200, avg=1850, max=3500

Testing Different Scenarios
****************************

Maximum Throughput Test
=======================

.. code-block:: console

   throughput set_size 1472
   throughput set_delay 0
   throughput start

Low Latency Test
================

.. code-block:: console

   throughput set_size 64
   throughput set_delay 10
   throughput start

Sustained Load Test
===================

.. code-block:: console

   throughput set_size 512
   throughput set_duration 300
   throughput start

