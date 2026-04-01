.. zephyr:code-sample:: spi_fujitsu_fram
   :name: SPI Fujitsu FRAM
   :relevant-api: spi_interface

   Read and write data on a Fujitsu MB85RS64V FRAM over SPI.

Overview
********

This sample shows how to talk to a Fujitsu MB85RS64V ferroelectric RAM (FRAM)
chip using the Zephyr SPI driver API. It runs through a simple test sequence:

#. Reads the manufacturer and product ID to confirm the FRAM is wired up
   correctly (expects ``04 7f 03 02``).
#. Does single-byte writes to addresses ``0x00`` and ``0x01``, then reads
   them back.
#. Fills a 1024-byte buffer with pseudo-random data, writes it to the FRAM
   starting at address ``0x00``, reads it back, and compares.

The SPI clock is set to 256 kHz with an 8-bit word size.

Requirements
************

- A board with SPI and GPIO support and a devicetree alias ``spi-1``.
- A Fujitsu MB85RS64V FRAM module wired to the SPI-1 bus.

Wiring
******

Connect the FRAM module to your board's SPI-1 pins:

+------------+----------------+
| FRAM Pin   | Board SPI-1    |
+============+================+
| SCK        | SCLK           |
+------------+----------------+
| SI (MOSI)  | MOSI           |
+------------+----------------+
| SO (MISO)  | MISO           |
+------------+----------------+
| CS#        | CS (GPIO)      |
+------------+----------------+
| VCC        | 3.3 V          |
+------------+----------------+
| GND        | GND            |
+------------+----------------+

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/spi_fujitsu_fram
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   fujitsu FRAM example application
   Wrote 0xAE to address 0x00.
   Wrote 0x86 to address 0x01.
   Read 0xAE from address 0x00.
   Read 0x86 from address 0x01.
   Wrote 1024 bytes to address 0x00.
   Read 1024 bytes from address 0x00.
   Data comparison successful.
