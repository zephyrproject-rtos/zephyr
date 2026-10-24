.. _romstart_flash_base:

Rom start at flash base with code partition
###########################################

Overview
********

A test for the combination of :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION`
and :kconfig:option:`CONFIG_ROMSTART_RELOCATION_ROM` with
:kconfig:option:`CONFIG_ROMSTART_REGION_AT_FLASH_BASE`.

The boot vectors are kept at the base of the flash region, where the SoC
boots from, while the application image is linked into the DT chosen
``zephyr,code-partition``. The flash in between is left uncovered by the
image and hosts the storage partition, so flashing a new application
image does not erase the stored data.

Building and Running
********************

The test is meant to be built and run on an STM32F4 based nucleo board,
whose flash sector layout (small initial sectors) is the typical use
case. It should also build for any Cortex-M board with a
``zephyr,code-partition`` defined.

The test verifies the location of the vector table and the application
image, and round-trips a settings entry on the storage partition.
