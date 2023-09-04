.. _mcuboot_sysbuild_test:

Hello World MCUBoot with Sysbuild
#################################

Overview
********

A simple test application to test the build and execution of MCUBoot on espressif boards.
The build is done using sysbuild tag and enabling CONFIG_BOOTLOADER_MCUBOOT.

Supported Boards
****************
- esp32_devkitc_wrover
- esp32c3_devkitm
- esp32s2_saola
- esp32s3_devkitm

Building and Running
********************

Make sure you have the target connected over USB port.

.. code-block:: console

   west build -b <board> tests/boards/espressif_esp32/mcuboot_sysbuild --sysbuild
   west flash && west espressif monitor

Sample Output
=============

.. code-block:: console

    I (49) boot: MCUboot 2nd stage bootloader
    I (49) boot: chip revision: v0.0
    I (49) boot.esp32s2: SPI Speed      : 40MHz
    I (49) boot.esp32s2: SPI Mode       : DIO
    I (52) boot.esp32s2: SPI Flash Size : 4MB
    I (56) boot: Enabling RNG early entropy source...
    I (72) spi_flash: detected chip: generic
    I (72) spi_flash: flash io: fastrd
    [esp32s2] [INF] Image index: 0, Swap type: none
    [esp32s2] [INF] Loading image 0 - slot 0 from flash, area id: 1
    [esp32s2] [INF] Application start=40024E54h
    [esp32s2] [INF] DRAM segment: paddr=00004D68h, vaddr=3FFB8590h, size=0033Ch (   828) load
    [esp32s2] [INF] IRAM segment: paddr=00001024h, vaddr=40022000h, size=03D44h ( 15684) load
    [esp32s2] [INF] DROM segment: paddr=00010040h, vaddr=3F000040h, size=00FF0h (  4080) map
    [esp32s2] [INF] IROM segment: paddr=00020000h, vaddr=40090000h, size=03398h ( 13208) map
    *** Booting Zephyr OS build zephyr-v3.4.0-2985-g01e17f19c149 ***
    Hello World! esp32s2_saola
