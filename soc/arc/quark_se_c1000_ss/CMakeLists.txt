zephyr_include_directories(${ZEPHYR_BASE}/soc/x86/intel_quark)

zephyr_cc_option(-mcpu=quarkse_em -mno-sdata)

zephyr_compile_definitions_ifdef(
  CONFIG_SOC_QUARK_SE_C1000_SS
  QM_SENSOR=1
  SOC_SERIES=quark_se
  )

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            soc.c
            soc_config.c
            power.c
            soc_power.S
)

target_sources(soc_arc PRIVATE ${PRIVATE_SOURCES})
target_include_directories(soc_arc PRIVATE ${ZEPHYR_BASE}/drivers)
