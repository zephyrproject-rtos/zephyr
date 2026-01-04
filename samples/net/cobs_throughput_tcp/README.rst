.. _slip_throughput_tcp:

SLIP TCP Throughput Test
#########################

Overview
********

This sample demonstrates TCP throughput testing over SLIP (Serial Line
Internet Protocol) using the COBS-encoded UART async driver on nRF5340.
The test creates two COBS interfaces on cross-wired UARTs and measures
TCP stream performance with a simple echo pattern.

Features:

* TCP connection between two COBS interfaces
* Continuous data stream with incrementing byte pattern (0x00-0xFF wrap)
* Configurable chunk size and test duration
* Real-time progress reporting (every second)
* Thread-safe statistics tracking
* Simple shell commands for control

Hardware Setup
**************

nRF5340 DK connections:

* Connect P1.09 (UART2 TX) -> P0.28 (UART3 RX)
* Connect P1.08 (UART2 RX) -> P0.29 (UART3 TX)

Building and Running
********************

Build for nRF5340 DK:

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp

Flash and monitor:

.. code-block:: console

   west flash
   west attach

Usage
*****

Available shell commands:

.. code-block:: console

   throughput status                    - Show current configuration and statistics
   throughput set_chunk_size 1024       - Set data chunk size (16-4096 bytes)
   throughput set_duration 30           - Set test duration (1-3600 seconds)
   throughput set_delay 10              - Set client delay between chunks (0-10000 ms)
   throughput set_processing_delay 100  - Set server processing delay (0-10000 ms)
   throughput start                     - Start throughput test
   throughput stop                      - Stop test and show final results

Example session:

.. code-block:: console

   uart:~$ throughput set_chunk_size 2048
   Chunk size set to 2048 bytes
   
   uart:~$ throughput set_duration 30
   Test duration set to 30 seconds
   
   uart:~$ throughput set_delay 10
   Client delay between chunks set to 10 ms
   
   uart:~$ throughput set_processing_delay 100
   Server processing delay set to 100 ms
   
   uart:~$ throughput start
   === Starting TCP Throughput Test ===
   Chunk size: 2048 bytes
   Duration: 30 seconds
   
   Progress: 1s | TX: 512 KB (4096 kbps) | RX: 512 KB (4096 kbps) | Errors: 0
   Progress: 2s | TX: 1024 KB (4096 kbps) | RX: 1024 KB (4096 kbps) | Errors: 0
   ...

Test automatically stops after configured duration or can be stopped manually:

.. code-block:: console

   uart:~$ throughput stop
   
   === Final Statistics ===
   Test duration: 30 seconds
   Chunk size: 2048 bytes
   
   Client TX: 15728640 bytes (15360 KB) (4194 kbps)
   Client RX: 15728640 bytes (15360 KB) (4194 kbps)
   Server Echo: 15728640 bytes (15360 KB)
   
   Errors: 0
   ========================

Architecture
************

The test consists of:

* **Server thread**: Listens for TCP connection, receives data and echoes
  it back (6KB stack)
* **Client thread**: Connects to server, sends data stream and verifies
  echo responses (10KB stack - needs space for 8KB of buffers)
* **Progress reporter**: Prints statistics every second via work queue

Data pattern:

* Simple incrementing byte counter (0x00, 0x01, ..., 0xFF, 0x00, ...)
* Each chunk filled with sequential bytes
* Pattern verification on receive to detect corruption

Performance characteristics:

* **Ping-pong pattern**: Client sends a chunk, waits for echo, then sends next
  chunk. This means throughput is limited by round-trip time (RTT).
* **Configurable client delay**: Default 0ms delay between chunks. Increase to
  throttle transmission rate.
* **Configurable server processing delay**: Default 0ms. Set to simulate server
  processing work between receiving and echoing data.
* **TCP optimizations**: Uses ``TCP_NODELAY`` to disable Nagle's algorithm for
  lower latency
* **Expected throughput**: Limited by RTT, TCP windowing, client delay, and
  server processing delay. Lower than a streaming send-only test

Thread safety:

* Statistics protected by mutex
* Atomic state management
* Clean thread join on stop

TCP shutdown sequence:

1. Client sends all data chunks for configured duration
2. Client calls ``shutdown(SHUT_WR)`` to send FIN packet
3. Client waits briefly (100ms) for FIN to be processed
4. Client closes its socket
5. Server receives EOF (recv returns 0) and stops echo loop
6. Server closes accepted connection and listening socket
7. Both threads signal completion via flags
8. Progress handler detects completion and auto-stops test
9. Final statistics are printed

Configuration
*************

Key Kconfig options:

* ``CONFIG_NET_TCP`` - Enable TCP support
* ``CONFIG_COBS_UART_ASYNC`` - SLIP UART async driver
* ``CONFIG_NET_L2_COBS_SERIAL`` - COBS encoding for SLIP
* ``CONFIG_NET_TCP_WINDOW_SIZE`` - TCP window size (4096 bytes)

Tuning:

* Increase chunk size for higher throughput
* Adjust ``CONFIG_NET_BUF_DATA_SIZE`` for larger buffers
* Modify UART speed in overlay (default: 1 Mbaud)

