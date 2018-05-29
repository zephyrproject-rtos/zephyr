zephyr_include_directories(
  .
  inc
  driverlib
  )
zephyr_compile_definitions(
  USE_CC3220_ROM_DRV_API
  )

zephyr_library()
zephyr_library_compile_definitions(${COMPILER})
# pin.c required by ti/drivers/spi/SPICC32XXDMA.c
# prcm.c required by rom_patch.h (i.e., can't use ROM version).
# utils.c required by prcm.c (calling UtilsDelay).
zephyr_library_sources(
  driverlib/utils.c
  driverlib/prcm.c
  driverlib/pin.c
  )
