spi ram test
############

Tests a SPI controller driver against a SPI-attached RAM (or FRAM) module,
performing write and read-back operations.

Exercises the core SPI controller APIs: write, transceive, and the WREN
protocol sequence used by standard SPI FRAM/SRAM devices.

The test targets devices that follow the standard SPI FRAM opcode set
(WREN, WRDI, RDSR, WRITE, READ), which is compatible with many common
SPI non-volatile and ferroelectric RAM chips (e.g. FM25W256, CY15B256,
MB85RS256).

Hardware Setup
==============

- Target board with a SPI controller.
- A SPI RAM or FRAM device connected to the SPI bus. Connect the device
  signals to the SPI bus pins as defined by the board's devicetree overlay.

The device must support SPI mode 0 (CPOL=0, CPHA=0) or mode 3, with
MSB-first byte order.

Running
=======

.. code-block:: console

   west twister -T tests/drivers/spi/spi_ram -p <your-board>

Test Cases
==========

- **test_ram_write_read** — Writes a known payload to the device and reads it
  back, verifying the data matches.

- **test_ram_read_status** — Reads the device status register via the RDSR
  opcode and confirms the Write Enable Latch (WEL) is clear at startup.

- **test_ram_write_enable_disable** — Issues WREN and verifies the WEL bit
  is set in the status register, then issues WRDI and verifies it clears.
