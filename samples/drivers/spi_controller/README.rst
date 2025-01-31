.. zephyr:code-sample:: spi-controller
   :name: SPI controller
   :relevant-api: spi_interface

   This example works on loopback mode and uses the spi_transceive and spi_transceive_cb apis.

Overview
********
This sample demonstrates using the spi_transceive and spi_transceive_cb APIs.
SPI works on loopback mode for both spi_transceive and spi_transceive_cb.

Building and Running
********************

The application will build only for a target that has a devicetree node spi0 in the devicetree.
Add corresponding overlay file for the target device.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/spi_controller
   :board: same54_xpro
   :goals: build flash
