zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_compile_definitions_ifdef(
  CONFIG_SOC_QUARK_SE_C1000
  QM_LAKEMONT
  SOC_SERIES=quark_se
  )

zephyr_cc_option(-march=lakemont -mtune=lakemont -msoft-float)

zephyr_sources(
  soc.c
  soc_config.c
  eoi.c
  power.c
  soc_power.S
  )
