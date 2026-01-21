.. _cobs_uart_async_sockets_test:

COBS UART Async Sockets Test
#############################

Overview
********

This test verifies the complete SLIP UART async networking stack including:

- Two SLIP network interfaces on a single board
- UDP socket communication between interfaces
- COBS framing and unframing
- Packet pattern verification
- Round-trip packet echo testing

The test sends 100 packets (512 bytes each) from interface 1 to interface 0,
which echoes them back, verifying data integrity throughout the stack.

Single-Board Testing with Loopback Wiring
******************************************

This test is designed to run on a **single development board** with loopback
wiring between two UART interfaces. This allows you to test the complete
networking stack without needing a second board.

Hardware Requirements
=====================

- 1x nRF5340DK (or other board with multiple UARTs)
- 2x female-to-female jumper wires
- Optional: Oscilloscope for debugging

nRF5340DK Wiring Instructions
==============================

The test uses two UART interfaces on the nRF5340DK:

- **UART2**: P1.08 (RX), P1.09 (TX)
- **UART3**: P0.28 (RX), P0.29 (TX)

Cross-Wire Configuration
------------------------

**IMPORTANT**: Wire the UARTs in a CROSS pattern so each interface can talk
to the other, NOT self-loopback!

.. code-block:: none

   UART2 → UART3:
   P1.09 (UART2_TX) --> P0.28 (UART3_RX)    [Use one jumper wire]

   UART3 → UART2:
   P0.29 (UART3_TX) --> P1.08 (UART2_RX)    [Use one jumper wire]

**Why cross-wiring?** Self-loopback (TX→RX on same UART) causes packets to
be received by the sending interface, potentially creating infinite loops.
Cross-wiring ensures packets go from cobs0 to cobs1 and vice versa.

**Physical Pin Locations:**

Pins are located on different headers:

.. code-block:: none

   P24 Header (Arduino-compatible):
   
   Pin#  Function
   ----  --------
   ...
   P1.08 UART2_RX  ← Connect to P1.09
   P1.09 UART2_TX  ← Connect to P1.08
   ...
   
   P2 Header:
   
   Pin#  Function
   ----  --------
   ...
   P0.28 UART3_RX  ← Connect to P0.29
   P0.29 UART3_TX  ← Connect to P0.28
   ...

Wiring Diagram
--------------

.. code-block:: none

            nRF5340DK Board
   +-------------------------------+
   |  P24 Header                   |
   |  ...                          |
   |  P1.08 [RX2]---+              |
   |                |              |
   |  P1.09 [TX2]---|-----------+  |
   |  ...           |           |  |
   |                |           |  |
   |  P2 Header     |           |  |
   |  ...           |           |  |
   |  P0.28 [RX3]<--+           |  |
   |                            |  |
   |  P0.29 [TX3]---------------+  |
   |  ...                          |
   +-------------------------------+
   
   Cross-wiring ensures bidirectional communication:
   - cobs0 (UART2) sends → cobs1 (UART3) receives
   - cobs1 (UART3) sends → cobs0 (UART2) receives

Step-by-Step Wiring
-------------------

1. **Power off the board**: Disconnect USB

2. **UART2 TX to UART3 RX** (cobs0 → cobs1):
   - Take one jumper wire (e.g., red)
   - Connect one end to P1.09 (UART2_TX) on P24 header
   - Connect other end to P0.28 (UART3_RX) on P2 header

3. **UART3 TX to UART2 RX** (cobs1 → cobs0):
   - Take another jumper wire (e.g., blue)
   - Connect one end to P0.29 (UART3_TX) on P2 header
   - Connect other end to P1.08 (UART2_RX) on P24 header

4. **Verify connections**: 
   - Check wires are firmly seated
   - Verify cross-pattern (not self-loopback!)

5. **Power on**: Reconnect USB

Building and Running
********************

For nRF5340DK
=============

.. code-block:: console

   # Navigate to Zephyr directory
   cd applications/concurrency/zephyr

   # Build the test
   west build -b nrf5340dk/nrf5340/cpuapp tests/drivers/cobs_uart_async_sockets

   # Flash to board
   west flash

   # Monitor output
   screen /dev/ttyACM0 115200

For Native Sim
==============

.. code-block:: console

   # Build and run
   west build -b native_sim tests/drivers/cobs_uart_async_sockets
   ./build/native_sim/cobs_uart_async_sockets/zephyr/zephyr.exe

Expected Output
***************

The test will perform the following sequence:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   Running TESTSUITE slip_sockets
   ===================================================================
   [00:00:00.100] <inf> slip_sockets_test: Setting up SLIP sockets test...
   [00:00:00.101] <inf> slip_sockets_test: Searching for COBS interfaces...
   [00:00:00.102] <inf> slip_sockets_test: Found COBS interface 1: cobs@0
   [00:00:00.103] <inf> slip_sockets_test: Assigned to if0_ctx (cobs0)
   [00:00:00.104] <inf> slip_sockets_test: Found COBS interface 2: cobs@1
   [00:00:00.105] <inf> slip_sockets_test: Assigned to if1_ctx (cobs1)
   [00:00:00.106] <inf> slip_sockets_test: Configured cobs0 with IP: 192.168.7.1
   [00:00:00.107] <inf> slip_sockets_test: Configured cobs1 with IP: 192.168.7.2
   [00:00:00.108] <inf> slip_sockets_test: Both interfaces are up and configured
   ===================================================================
   START - test_interfaces_configured
    PASS - test_interfaces_configured in 0.001 seconds
   ===================================================================
   START - test_ip_addresses
    PASS - test_ip_addresses in 0.001 seconds
   ===================================================================
   START - test_udp_echo
   [00:00:01.000] <inf> slip_sockets_test: Server thread started
   [00:00:01.001] <inf> slip_sockets_test: Server listening on port 5001
   [00:00:01.500] <inf> slip_sockets_test: === UDP Echo Test ===
   [00:00:01.501] <inf> slip_sockets_test: Sending 100 packets of 512 bytes
   [00:00:01.502] <inf> slip_sockets_test: Client sending to server at 192.168.7.1:5001
   [00:00:03.000] <inf> slip_sockets_test: Progress: 20/100 packets
   [00:00:04.500] <inf> slip_sockets_test: Progress: 40/100 packets
   [00:00:06.000] <inf> slip_sockets_test: Progress: 60/100 packets
   [00:00:07.500] <inf> slip_sockets_test: Progress: 80/100 packets
   [00:00:09.000] <inf> slip_sockets_test: Progress: 100/100 packets
   [00:00:09.500] <inf> slip_sockets_test: === Test Results ===
   [00:00:09.501] <inf> slip_sockets_test: Packets sent: 100
   [00:00:09.502] <inf> slip_sockets_test: Packets received: 100
   [00:00:09.503] <inf> slip_sockets_test: Bytes sent: 51200
   [00:00:09.504] <inf> slip_sockets_test: Bytes received: 51200
   [00:00:09.505] <inf> slip_sockets_test: Errors: 0
   [00:00:09.506] <inf> slip_sockets_test: Test passed!
    PASS - test_udp_echo in 8.006 seconds
   ===================================================================
   TESTSUITE slip_sockets succeeded

   PROJECT EXECUTION SUCCESSFUL

What the Test Verifies
***********************

1. **Interface Discovery**: Both COBS interfaces are detected
2. **IP Configuration**: IPv4 addresses assigned correctly
3. **Socket Creation**: UDP sockets can be created and bound
4. **Packet Transmission**: Data flows through SLIP/COBS/UART layers
5. **Loopback**: UART TX→RX loopback works correctly
6. **COBS Framing**: Encoding/decoding preserves data integrity
7. **Pattern Verification**: All packets maintain correct data patterns
8. **Full Stack**: Complete networking stack from sockets to UART

Troubleshooting
***************

Test Fails or Hangs
===================

**Issue**: Test times out or fails to complete

**Solutions**:

1. **Check wiring** (must be cross-wired, NOT self-loopback):
   - Verify P1.09 (UART2 TX) connects to P0.28 (UART3 RX)
   - Verify P0.29 (UART3 TX) connects to P1.08 (UART2 RX)
   - Ensure good electrical contact
   - **DO NOT** wire P1.09→P1.08 or P0.29→P0.28 (causes loops!)

2. **Verify UART configuration**:
   - Check both UARTs are enabled in overlay
   - Confirm correct pins in pinctrl

3. **Check logs**:
   - Look for UART initialization errors
   - Check for COBS encoding/decoding errors

No SLIP Interfaces Found
========================

**Issue**: Test reports "Expected 2 COBS interfaces, found 0"

**Solutions**:

1. Verify devicetree overlay is applied:

   .. code-block:: console

      # Check generated devicetree
      west build -t menuconfig
      # Or examine build/zephyr/zephyr.dts

2. Ensure UART devices are available
3. Check Kconfig options are enabled

Pattern Verification Failures
==============================

**Issue**: Test reports "Pattern verification failed"

**Causes**:
- Poor wire connections (intermittent contact)
- Wire too long (>30cm causes signal integrity issues)
- Electromagnetic interference

**Solutions**:
- Use shorter jumper wires
- Ensure secure connections
- Move away from noise sources (motors, power supplies)

High Error Rate
===============

**Issue**: Test passes but reports many errors (>10%)

**Solutions**:
- Increase packet delay in test code
- Reduce buffer sizes if memory constrained
- Lower UART baud rate for stability

Performance Expectations
************************

On nRF5340DK with 115200 baud loopback:

- **Throughput**: ~50-80 kbps per interface
- **Latency**: 20-50ms round-trip time
- **Packet loss**: <1% (should be 0% in loopback)
- **COBS overhead**: ~0.4% additional bytes

Factors affecting performance:

1. **UART baud rate**: 115200 is default
2. **Packet size**: 512 bytes default
3. **Processing overhead**: Zephyr networking stack
4. **Wire quality**: Should have minimal impact in loopback

Customization
*************

Change Packet Size
==================

Edit ``src/main.c``:

.. code-block:: c

   #define TEST_PACKET_SIZE 1024  // Change from 512

Change Packet Count
===================

Edit ``src/main.c``:

.. code-block:: c

   #define TEST_PACKET_COUNT 500  // Change from 100

Change IP Addresses
===================

Edit ``src/main.c``:

.. code-block:: c

   static struct iface_context if0_ctx = {
       .ip_addr = "10.0.0.1",  // Change IP
       .name = "cobs0",
   };

Change UART Baud Rate
=====================

Edit board overlay (e.g., ``nrf5340dk_nrf5340_cpuapp.overlay``):

.. code-block:: devicetree

   &uart2 {
       current-speed = <230400>;  // Double the speed
       ...
   };

   &uart3 {
       current-speed = <230400>;
       ...
   };

Use Different GPIO Pins
========================

Edit the pinctrl section in board overlay:

.. code-block:: devicetree

   uart2_default: uart2_default {
       group1 {
           psels = <NRF_PSEL(UART_TX, 1, 5)>,  // Different pins
                   <NRF_PSEL(UART_RX, 1, 4)>;
       };
   };

Advanced Testing
****************

Stress Test
===========

Modify test to run continuously:

.. code-block:: c

   // In test_udp_echo(), wrap main loop:
   for (int iteration = 0; iteration < 10; iteration++) {
       LOG_INF("Iteration %d/10", iteration + 1);
       // ... existing packet sending loop ...
   }

Performance Measurement
=======================

Add timing measurements:

.. code-block:: c

   uint32_t start_time = k_uptime_get_32();
   // ... run test ...
   uint32_t elapsed = k_uptime_get_32() - start_time;
   uint32_t throughput_bps = (test_stats.bytes_sent * 8 * 1000) / elapsed;
   LOG_INF("Throughput: %u bps", throughput_bps);

Integration with CI/CD
======================

This test is suitable for automated testing:

.. code-block:: console

   # Run with timeout
   west build -b native_sim tests/drivers/cobs_uart_async_sockets
   timeout 60 ./build/native_sim/cobs_uart_async_sockets/zephyr/zephyr.exe

   # Check exit code
   echo $?  # Should be 0 for success

Limitations
***********

1. **Loopback only**: Tests single-board loopback, not real TX/RX separation
2. **Timing**: Self-loopback has different timing than separate boards
3. **Buffer pressure**: Both TX and RX share same CPU/memory
4. **No cable effects**: Doesn't test real cable characteristics

For full validation, also test with the throughput client/server samples on
two separate boards with a physical cable connection.

See Also
********

- :ref:`cobs_throughput_server` - Server sample for two-board testing
- :ref:`cobs_throughput_client` - Client sample for two-board testing
- ``samples/net/cobs_uart_async`` - Basic SLIP UART usage example
- ``tests/drivers/cobs_uart_async_loopback`` - Lower-level driver test

