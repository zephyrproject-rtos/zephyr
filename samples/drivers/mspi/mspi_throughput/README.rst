.. zephyr:code-sample:: mspi-throughput
   :name: MSPI throughput performance test
   :relevant-api: mspi_interface

   Measure MSPI throughput performance with different buffer sizes and packet counts.

Overview
********

This sample demonstrates MSPI throughput performance testing by measuring
transfer rates across different buffer sizes and packet configurations. The
sample performs a series of tests to determine optimal MSPI transfer
parameters for maximum throughput and efficiency.

The sample tests:
* Multiple buffer sizes: 4, 16, 64, 256, 1024, 1500, and 2304 bytes
* Multiple packet counts: 1, 4, and 16 packets per transfer
* Total transfer size: 32 KB per test case
* Performance metrics: throughput (MB/s), efficiency (%), CPU load (%), and latency.

NOTE: The latency is calculated based on the efficiency loss and is an estimated value and is
not printed in the summary table, it is printed in the detailed results for each test case.

The sample supports both DMA and CPU-driven polling modes. When
``CONFIG_CPU_LOAD`` is enabled, it reports CPU load (percentage) using the
:ref:`cpu_load` subsystem (supported on Cortex-M, RISC-V, and Cortex-A;
single-core only).

Goals and future work
*********************

The current sample measures **controller-side** throughput and efficiency so
that you can see how the driver performs with different packet sizes and
packet counts, and compare DMA vs FIFO and CPU utilization. This is useful
for characterizing any MSPI controller regardless of what is on the other end
(flash, slave, or host).

In the future, the sample may be extended so that two SoCs
run it—one in controller mode and one in peripheral mode—with a simple
protocol to measure full link throughput. That work depends on a defined
protocol and MSPI peripheral support.

Configuration
*************

The sample is configured for:

MSPI device parameters (clock frequency, I/O mode, data rate, endianness, command/address length,
etc.) come from devicetree on the target device node via ``MSPI_DEVICE_CONFIG_DT()``. See the
board overlay for the specific values.

Total transfer size per test case is set by Kconfig
``CONFIG_SAMPLE_MSPI_THROUGHPUT_TOTAL_TRANSFER_SIZE`` (default: 32 KB).

Hardware Setup
**************

This sample requires a board overlay that defines alias ``dev0`` to the MSPI
target device node (e.g. a dummy or peripheral device with ``reg = <0>`` under
the controller). The sample obtains the controller as the bus of ``dev0`` and
uses ``MSPI_DEVICE_ID_DT(DT_ALIAS(dev0))`` for the device index and chip-select.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/mspi/mspi_throughput
   :board: nrf7120dk_nrf7120_cpuapp
   :goals: Test MSPI throughput
   :compact:


Sample Output
=============

The sample outputs detailed performance metrics for each test configuration.

Example output of running nRF7120 at 32MHz clock for 32KB total transfer size:

.. code-block:: console

=== TEST RESULTS ===
BufSize  PktCnt   Time         Throughput   Efficiency   CPU Load
(bytes)           (us)         (MB/s)       (%)          (%)
-------------------------------------------------------------------
4        1        87432        0.536        3.51         100.00
4        4        79175        0.444        2.91         100.00
4        16       76135        0.423        2.77         100.00
16       1        31622        1.112        7.29         100.00
16       4        28646        1.125        7.37         100.00
16       16       28037        1.123        7.36         100.00
64       1        7910         4.074        26.70        99.99
64       4        7158         4.400        28.83        99.99
64       16       6956         4.501        29.50        99.99
256      1        3610         8.724        57.17        71.36
256      4        3460         9.049        59.31        69.28
256      16       3424         9.131        59.84        68.87
1024     1        2441         12.827       84.06        26.50
1024     4        2403         13.011       85.27        25.00
1024     16       2393         13.061       85.59        24.63
1500     1        2319         13.494       88.43        19.20
1500     4        2295         13.622       89.27        18.15
1500     16       2289         13.654       89.48        17.89
2304     1        2234         14.001       91.76        13.64
2304     4        2218         14.093       92.36        12.83
2304     16       2212         14.128       92.59        12.55


=== SUMMARY REPORT ===
Best TX Performance: 14.13 MB/s (buffer size=2304, packet count=16)
=== MSPI Throughput Test Complete ===

The output includes:
* **BufSize**: Buffer size in bytes used for each transfer
* **PktCnt**: Number of packets per transfer
* **Time**: Total transfer time in microseconds (CPU cycles)
* **Throughput**: Measured throughput in MB/s
* **Efficiency**: Percentage of ideal throughput achieved
* **CPU Load**: CPU utilization percentage (when ``CONFIG_CPU_LOAD`` is enabled)

The summary report identifies the configuration that achieved the highest
throughput.

Performance Analysis
====================

The sample calculates several performance metrics:

* **Throughput**: Actual data transfer rate in MB/s, calculated from total
  bytes transferred (including command and address overhead) divided by
  transfer time.

* **Efficiency**: Percentage of ideal throughput achieved. Ideal throughput
  is calculated as (clock_frequency * 4) / 8 for quad mode, accounting for
  4 data lines and 8 bits per byte.

* **CPU Load**: When ``CONFIG_CPU_LOAD`` is enabled, the sample uses the
  CPU load subsystem to measure utilization during transfers (architecture-agnostic
  where supported).

* **Estimated Latency**: Calculated per-buffer overhead based on efficiency
  loss, helping identify optimal buffer sizes.

The sample helps identify:
* Optimal buffer sizes for maximum throughput
* Impact of packet batching on performance
* CPU overhead of different transfer configurations
