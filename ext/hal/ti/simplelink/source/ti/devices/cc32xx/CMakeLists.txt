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
zephyr_library_sources(
  driverlib/utils.c
  driverlib/prcm.c
  driverlib/pin.c
  )
