target_include_directories(soc_x86 PRIVATE ${ZEPHYR_BASE}/drivers)

zephyr_compile_definitions_ifdef(
  CONFIG_SOC_QUARK_D2000
  QM_LAKEMONT
  SOC_SERIES=quark_d2000
  )

zephyr_cc_option(-march=lakemont -mtune=lakemont -msoft-float)

target_sources(soc_x86 PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)
