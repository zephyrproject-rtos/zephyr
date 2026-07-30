.. zephyr:code-sample:: spi-rtio-loopback
   :name: SPI RTIO loopback
   :relevant-api: rtio spi_interface

   Perform SPI transfers between SPI controller and SPI target using RTIO.

Overview
********

This sample demonstrates how to perform SPI transfers using RTIO. It uses two SPI controllers,
acting as SPI controller and peripheral/target respectively.

Requirements
************

This sample requires two SPI controllers, one supporting the SPI controller role, one supporting
the SPI peripheral/target role, connected to the same SPI bus.

.. note::

   Remember to connect the relevant SPI controllers to the bus physically.

Board support
*************

Any board which meets the requirements must use an overlay to specify which SPI controller will act
as the controller, and which as the peripheral/target. This is done using the devicetree by adding
SPI device nodes to the relevant SPI controllers, adding the nodelabel ``spi_controller`` to the
SPI device SPI controller acting as controller, and ``spi_target`` to the SPI device SPI controller
acting as peripheral/target. These two SPI device nodes describe the same SPI device, from the
perspective of the SPI controller and SPI peripheral/target respectively.

.. code-block:: devicetree

   &spi22 {
           status = "okay";
           cs-gpios = <&gpio2 10 GPIO_ACTIVE_LOW>;

           spi_controller: spi-device@0 {
                      compatible = "vnd,spi-device";
                      reg = <0>;
                      spi-max-frequency = <DT_FREQ_M(8)>;
           };
   };

   &spi21 {
           status = "okay";

           spi_target: spi-device@0 {
                      compatible = "vnd,spi-device";
                      reg = <0>;
                      spi-max-frequency = <DT_FREQ_M(8)>;
           };
   };

If necessary, add any board specific configs to the board specific conf fragment:

.. code-block:: cfg

   CONFIG_SPI_STM32_INTERRUPT=y

Transferred data sizes
**********************

One can tune the number of data bytes exchanged during the tests using the sample specific
configuration options:

* :kconfig:option:`CONFIG_SAMPLE_DATA_WRITE_SIZE`
* :kconfig:option:`CONFIG_SAMPLE_DATA_READ_SIZE`
* :kconfig:option:`CONFIG_SAMPLE_SPI_MODE_0`
* :kconfig:option:`CONFIG_SAMPLE_SPI_MODE_1`
* :kconfig:option:`CONFIG_SAMPLE_SPI_MODE_2`
* :kconfig:option:`CONFIG_SAMPLE_SPI_MODE_3`
* :kconfig:option:`CONFIG_SAMPLE_SPI_BIT_ORDER_MSB`
* :kconfig:option:`CONFIG_SAMPLE_SPI_BIT_ORDER_LSB`
