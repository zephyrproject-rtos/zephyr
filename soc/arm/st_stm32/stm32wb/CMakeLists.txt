zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  )

zephyr_linker_sources_ifdef(CONFIG_BT_STM32_IPM
  SECTIONS
  ipm.ld
  )

zephyr_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT
  power.c
  )
