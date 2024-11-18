.. _ti-spi-nor-secondary-app:

TI SPI NOR Secondary Snippet for Application
############################################

.. code-block:: console

   west build -S ti-spi-nor-secondary-app [...]

Overview
********

This snippet increases the slot size of MCUboot partitions and sets the portion
of external flash as the secondary slot.

Requirements
************

- Support for the following
   - :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`
- Must be applied to an application that is meant to use MCUboot as the bootloader
- A devicetree node with node label ``mx25r80`` that points to a ``jedec,spi-nor``
  compatible SPI NOR flash
- A devicetree node with node label ``flash0`` that points to a ``soc-nv-flash``
  compatible SOC NV flash
