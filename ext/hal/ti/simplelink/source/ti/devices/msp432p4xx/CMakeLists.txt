zephyr_include_directories(
  .
  )

zephyr_library()
zephyr_library_compile_definitions(${COMPILER})
zephyr_library_sources(
  # Need system_msp432p401r for SystemInit which is not in ROM
  startup_system_files/system_msp432p401r.c
  )

