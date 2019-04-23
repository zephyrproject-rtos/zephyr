# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
  .
  )

zephyr_library()
zephyr_library_compile_definitions(${COMPILER})
zephyr_library_sources(
  # Required for SystemTrimDevice which is not in ROM
  driverlib/setup.c
  driverlib/chipinfo.c
  driverlib/aux_sysif.c
  # Required for CPUdelay which is not in ROM
  driverlib/cpu.c
  # Required for default CCFG confguration
  startup_files/ccfg.c
  )
