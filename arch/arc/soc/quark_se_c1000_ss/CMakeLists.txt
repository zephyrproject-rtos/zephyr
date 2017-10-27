zephyr_library_include_directories(${PROJECT_SOURCE_DIR}/drivers)
zephyr_include_directories(${PROJECT_SOURCE_DIR}/arch/x86/soc/intel_quark)

zephyr_cc_option(-mcpu=quarkse_em -mno-sdata)

zephyr_compile_definitions_ifdef(
  CONFIG_SOC_QUARK_SE_C1000_SS
  QM_SENSOR=1
  SOC_SERIES=quark_se
  )

zephyr_sources(
  soc.c
  soc_config.c
  power.c
  soc_power.S
  )
