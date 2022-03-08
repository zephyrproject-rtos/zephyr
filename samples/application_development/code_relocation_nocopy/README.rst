.. _code_relocation_nocopy:

Code relocation nocopy
######################

Overview
********
A simple example that demonstrates how relocation of code, data or bss sections
using a custom linker script.

Differently from the code relocation sample, this sample is relocating the
content of the ext_code.c file to a different FLASH section and the code is XIP
directly from there without the need to copy / relocate the code.

nRF5340dk platform instructions
*******************************

The nRF5340 PDK has a 64 Mb external flash memory supporting Quad SPI. It is
possible to do XIP from the external flash memory.

The external flash memory is mapped to 0x10000000.

In this sample we relocate some of the code to the external flash memory with
the remaining Zephyr kernel in the internal flash.

Compile the code:
  $ west build -b nrf5340dk_nrf5340_cpuapp samples/application_development/code_relocation_nocopy --pristine

Get the HEX file from the ELF:
  $ arm-linux-gnueabihf-objcopy -O ihex build/zephyr/zephyr.elf zephyr.hex

Erase the external FLASH (just to be sure):
  $ nrfjprog --qspicustominit --qspieraseall

Write the HEX to internal and external FLASH:
  $ nrfjprog --coprocessor CP_APPLICATION --sectorerase --qspisectorerase --program zephyr.hex

Execution output:

  *** Booting Zephyr OS build v3.0.0-rc3-25-g0df32cec1ff2  ***
  Address of main function 0x4f9
  Address of function_in_ext_flash 0x10000001
  Address of var_ext_sram_data 0x200000a0 (10)
  Address of function_in_sram 0x20000001
  Address of var_sram_data 0x200000a4 (10)
  Hello World! nrf5340dk_nrf5340_cpuapp
