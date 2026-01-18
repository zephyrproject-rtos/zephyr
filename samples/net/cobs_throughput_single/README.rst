.. _slip_throughput_single:

COBS Throughput Test (Single Device)
#####################################

Overview
********

This sample demonstrates throughput testing over SLIP (Serial Line Internet
Protocol) using UART with COBS encoding. Unlike separate client/server samples,
this combines both roles in a single application with **two COBS interfaces**
connected via cross-wired UARTs.

The sample creates three threads:
- **Server thread**: Echoes received UDP packets back (on cobs1/UART3)
- **Client TX thread**: Sends UDP packets with patterns (from cobs0/UART2)
- **Client RX thread**: Receives echo responses and verifies patterns

Data flows: cobs0 (UART2) → physical wires → cobs1 (UART3) → wires → cobs0

This is useful for:
- Testing complete SLIP/UART driver stack with real hardware
- Validating data integrity through physical UART connections
- Measuring throughput with various packet sizes at high baud rates
- Stress testing UART async API, DMA, and COBS encoding/decoding

Requirements
************

- A board with UART support (e.g., nRF5340 DK)
- For hardware testing: TX/RX pins cross-wired or two boards connected
- For native_sim: Uses PTY for testing

Building and Running
********************

nRF5340 DK
==========

To build for nRF5340 DK with cross-wired UART1:

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp

Flash the board:

.. code-block:: console

   west flash

Native Simulation
=================

For testing without hardware:

.. code-block:: console

   west build -b native_sim

Run:

.. code-block:: console

   ./build/zephyr/zephyr.exe

Usage
*****

The sample provides shell commands under the ``throughput`` command group:

Configuration Commands
======================

Configure test parameters before starting:

.. code-block:: console

   throughput status              # Show current configuration and stats
   throughput set_size 1024       # Set packet size (16-1472 bytes)
   throughput set_duration 30     # Set test duration (1-3600 seconds)
   throughput set_delay 10        # Set delay between packets (milliseconds)

Test Commands
=============

Start and stop tests:

.. code-block:: console

   throughput start               # Start throughput test
   throughput stop                # Stop test and print final statistics

Example Session
===============

.. code-block:: console

   uart:~$ throughput status
   Current configuration:
     Packet size: 512 bytes
     Duration: 60 seconds
     Delay: 0 ms
     Status: IDLE

   uart:~$ throughput set_size 1024
   Packet size set to 1024 bytes

   uart:~$ throughput set_duration 30
   Test duration set to 30 seconds

   uart:~$ throughput start
   Starting throughput test...
   === Starting Throughput Test ===
   Packet size: 1024 bytes
   Duration: 30 seconds
   Delay: 0 ms
   Test started...

   # ... test runs for 30 seconds or until stopped ...

   uart:~$ throughput stop
   Stopping throughput test...

   === Final Statistics ===
   Test duration: 30 seconds
   Packet size: 1024 bytes

   Client TX: 5000 packets, 5120000 bytes (1365 kbps)
   Client RX: 4985 packets, 5104640 bytes (1361 kbps)

   Server RX: 4985 packets
   Server TX: 4985 packets

   Packet loss: 15 (0.30%)
   Errors: 0, Pattern errors: 0, Timeouts: 15

   RTT (us): min=500, avg=650, max=1200
   ======================

Statistics
**********

The sample tracks comprehensive statistics:

Client Statistics
=================

- **Client TX**: Packets and bytes sent by client
- **Client RX**: Packets and bytes received (echo responses)
- **Packet loss**: Sent but not received packets
- **Pattern errors**: Received packets with incorrect data
- **Timeouts**: Receive timeouts

Server Statistics
=================

- **Server RX**: Packets received by echo server
- **Server TX**: Packets echoed back to client

Timing
======

- **RTT**: Round-trip time (minimum, average, maximum)
- **Throughput**: Calculated in kbps for TX and RX

Configuration Options
*********************

Kconfig Options
===============

- :kconfig:option:`CONFIG_SLIP_THROUGHPUT_PACKET_SIZE`: Default packet size (512 bytes)
- :kconfig:option:`CONFIG_SLIP_THROUGHPUT_DURATION_SEC`: Default test duration (60 seconds)
- :kconfig:option:`CONFIG_NET_BUF_DATA_SIZE`: Network buffer size (1500 bytes)
- :kconfig:option:`CONFIG_NET_PKT_RX_COUNT`: Receive packet count (32)
- :kconfig:option:`CONFIG_NET_PKT_TX_COUNT`: Transmit packet count (32)

Hardware Setup
**************

.. important::
   This sample requires **two cross-wired UARTs** for proper operation.
   See :ref:`cobs_throughput_single_hardware` for detailed setup instructions.

nRF5340 DK Cross-Wire Setup (Quick Start)
==========================================

For single-board testing on nRF5340 DK:

1. Connect **P1.09 (UART2 TX)** → **P0.28 (UART3 RX)** with a wire
2. Connect **P1.08 (UART2 RX)** ← **P0.29 (UART3 TX)** with a wire
3. Build and flash the application
4. Connect to serial console (UART0 via USB)
5. Use shell commands to run tests

.. note::
   The sample uses two UARTs at 1 Mbps:
   
   - **UART2** (cobs0): Client interface at fd00:1::1
   - **UART3** (cobs1): Server interface at fd00:2::1
   - **UART0**: Shell/console (USB CDC or P0.20/P0.22 pins)

Two-Board Setup
===============

For testing between two boards:

**Board 1 (Device Under Test):**

1. Flash the application
2. Note the TX/RX pins from the overlay

**Board 2 (Echo Device):**

1. Can use the same application or cobs_throughput_server
2. Cross-connect: Board1 TX → Board2 RX, Board1 RX → Board2 TX
3. Connect GND between boards

Troubleshooting
***************

Interface Not Coming Up
=======================

If the COBS interface fails to initialize:

- Check UART configuration in devicetree overlay
- Verify correct pins are assigned
- Check UART is enabled in devicetree
- For hardware loopback, verify physical connection

No Echo Responses
=================

If client sends but receives no echoes:

- Verify server thread started (check logs)
- Check UART TX/RX are properly connected
- Try reducing packet size
- Increase delay between packets
- Check for buffer exhaustion in statistics

High Packet Loss
================

If experiencing significant packet loss:

- Reduce packet size
- Add delay between packets
- Increase buffer counts in prj.conf
- Check UART baud rate is stable
- Verify power supply is adequate

See Also
********

- :ref:`cobs_uart_async_sample`
- :ref:`cobs_throughput_client`
- :ref:`cobs_throughput_server`
- :ref:`cobs_uart_async_sockets_test`

