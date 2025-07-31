.. _sdram_sample:

External SDRAM Sample
####################

Overview
********

This sample demonstrates external SDRAM access via the LPC54S018's External
Memory Controller (EMC). The sample performs various memory tests and bandwidth
measurements on the external SDRAM.

The EMC supports:
- SDR SDRAM up to 256MB
- 8-bit, 16-bit, or 32-bit data bus
- Up to 4 chip selects
- Programmable timing parameters
- Auto-refresh with programmable period

Hardware Requirements
********************

This sample requires an external SDRAM chip connected to the EMC interface.
The example configuration is for a W9812G6JB-6I (16MB, 16-bit) SDRAM.

SDRAM connections:
- Address lines: A0-A14 on P0.0-P0.14, P1.10-P1.13
- Data lines: D0-D15 on P0.4-P0.7, P1.0-P1.15
- Control signals: RAS, CAS, WE, CLK, CS, CKE, DQM

Building and Running
********************

.. code-block:: console

   west build -b lpcxpresso54s018 samples/drivers/sdram
   west flash

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0 ***
   [00:00:00.000,000] <inf> sdram_test: SDRAM test application for LPC54S018
   [00:00:00.001,000] <inf> sdram_test: SDRAM base: 0xA0000000, size: 16 MB
   [00:00:00.002,000] <inf> sdram_test: Running basic SDRAM test...
   [00:00:00.003,000] <inf> sdram_test: Basic SDRAM access OK!
   [00:00:00.004,000] <inf> sdram_test: Measuring SDRAM bandwidth...
   [00:00:00.050,000] <inf> sdram_test: Write bandwidth: 45 MB/s
   [00:00:00.100,000] <inf> sdram_test: Read bandwidth: 90 MB/s

Shell Commands
**************

The sample provides interactive shell commands:

.. code-block:: console

   uart:~$ sdram test
   Starting SDRAM test...
   Testing with pattern 0xAA55AA55...
   Writing 4194304 words...
   Verifying 4194304 words...
   Pattern 0xAA55AA55 test PASSED
   
   uart:~$ sdram fill 0x12345678
   Filling SDRAM with 0x12345678...
   SDRAM filled with 0x12345678
   
   uart:~$ sdram dump 0 16
   0xA0000000: 12345678 12345678 12345678 12345678
   0xA0000010: 12345678 12345678 12345678 12345678
   0xA0000020: 12345678 12345678 12345678 12345678
   0xA0000030: 12345678 12345678 12345678 12345678
   
   uart:~$ sdram bandwidth
   Measuring SDRAM bandwidth...
   Write bandwidth: 45 MB/s
   Read bandwidth: 90 MB/s

Test Patterns
*************

The test suite includes:
1. **Alternating pattern** (0xAA55AA55) - Tests all data lines
2. **Inverse pattern** (0x55AA55AA) - Opposite of pattern 1
3. **Incrementing pattern** - Each location contains its address
4. **Decrementing pattern** - Each location contains ~address
5. **Random access** - Tests random locations with random data

Configuration
*************

The SDRAM parameters are configured in the device tree overlay:

.. code-block:: dts

   &emc {
       status = "okay";
       nxp,emc-clock-div = <1>; /* EMC clock = CPU clock / 2 */
       
       sdram0: sdram@a0000000 {
           compatible = "nxp,lpc-emc-sdram";
           reg = <0xa0000000 0x1000000>; /* 16MB */
           
           /* Configuration for W9812G6JB-6I */
           nxp,sdram-config = <0 0 1 12 9>;
           nxp,sdram-timing = <18 42 67 18 45 12 60 60 67 12 2>;
           nxp,refresh-period = <64000>;
           nxp,cas-latency = <3>;
       };
   };

Performance Notes
*****************

- Write bandwidth is typically half of read bandwidth due to bus turnaround
- Performance depends on EMC clock frequency and SDRAM speed grade
- Burst mode provides better performance than single accesses
- Cache-aligned accesses improve performance

Troubleshooting
***************

If SDRAM access fails:
1. Check all connections, especially clock and control signals
2. Verify SDRAM power supply (typically 3.3V)
3. Ensure proper timing parameters for your SDRAM chip
4. Check EMC clock frequency (should not exceed SDRAM rating)
5. Verify chip select and address mapping configuration

Memory Mapping
**************

The SDRAM is mapped at 0xA0000000 in the memory space. The MPU is
configured to allow access to this region as normal memory (cacheable,
bufferable).

To use SDRAM for application data, add to your linker script:

.. code-block:: none

   MEMORY
   {
       SDRAM (rwx) : ORIGIN = 0xA0000000, LENGTH = 16M
   }
   
   SECTIONS
   {
       .sdram_data :
       {
           *(.sdram_data)
           *(.sdram_data.*)
       } > SDRAM
   }