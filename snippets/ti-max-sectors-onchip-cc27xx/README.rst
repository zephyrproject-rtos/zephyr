.. _ti-max-sectors-onchip-cc27xx:

TI Max Sectors Onchip CC27XX for MCUboot Bootloader
###################################################

.. code-block:: console

   west build -S ti-max-sectors-onchip-cc27xx [...]

Overview
********

This snippet sets the CONFIG_BOOT_MAX_IMG_SECTORS to an appropriate value for CC27XX
in order to maximize use of available flash space when both MCUboot slots are
onchip. It is intended to be used with the TI CC27XX SoC family.

Requirements
************

- Must be applied to the MCUboot bootloader project itself
