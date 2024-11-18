.. _ti-spi-nor-secondary-bl:

TI SPI NOR Secondary Snippet for MCUboot Bootloader
###################################################

.. code-block:: console

   west build -S ti-spi-nor-secondary-bl [...]

Overview
********

This snippet increases the slot size of MCUboot partitions and sets the external
flash as the secondary slot for the MCUboot bootloader image. It also applies
settings for using SPI NOR so that MCUboot can communicate properly with the
external flash.

Requirements
************

- Hardware support for the following
   - :kconfig:option:`CONFIG_FLASH`
   - :kconfig:option:`CONFIG_SPI`
   - :kconfig:option:`CONFIG_SPI_NOR`
- Must be applied to the MCUboot bootloader project itself
- A devicetree node with node label ``mx25r80`` that points to a ``jedec,spi-nor``
  compatible SPI NOR flash
- A devicetree node with node label ``flash0`` that points to a ``soc-nv-flash``
  compatible SOC NV flash
