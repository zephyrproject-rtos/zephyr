zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_compile_definitions_ifdef(
  CONFIG_SOC_QUARK_D2000
  QM_LAKEMONT
  SOC_SERIES=quark_d2000
  )

zephyr_cc_option(-march=lakemont -mtune=lakemont -msoft-float)
