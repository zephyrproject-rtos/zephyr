.. _spi_half_duplex_test:

SPI Half-Duplex Test
####################

Overview
********

This test suite verifies the functionality of the SPI half-duplex mode
on Zephyr-supported hardware. It ensures that the driver can correctly
handle transitions between transmission and reception on the same line.

Wiring
******

In this test suite, two instances of the SPI peripheral are needed on the
same device (i.e. spi21 and spi22, etc.).

For the test to pass, the following pins must be connected (see overlay
for nrf54l15dk for reference):

1. **SCK:** Connect ``spi22-SPIM_SCK`` with ``spi21-SPIS_SCK``
2. **MISO:** Connect ``spi22-SPIM_MISO`` with ``spi21-SPIS_MISO``
3. **MOSI:** Connect ``spi22-SPIM_MOSI`` with ``spi21-SPIS_MOSI``
4. **CS:** Connect ``spi22-cs-gpios`` with ``spi21-SPIS_CSN``

Building and Running
********************

To build and run this test on a supported board (e.g., Nucleo H743ZI):

.. code-block:: console

   west build -b nucleo_h743zi tests/drivers/spi/spi_half_duplex
   west flash
