zephyr_library_include_directories(${PROJECT_SOURCE_DIR}/drivers)

zephyr_compile_definitions_ifdef(
  CONFIG_SOC_QUARK_SE_C1000
  QM_LAKEMONT
  SOC_SERIES=quark_se
  )

zephyr_cc_option(-march=lakemont -mtune=lakemont -msoft-float)

if(CONFIG_X86_IAMCU)
  set_property(GLOBAL APPEND PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__IAMCU)
  set_property(GLOBAL        PROPERTY PROPERTY_OUTPUT_FORMAT         "elf32-iamcu")
  set_property(GLOBAL        PROPERTY PROPERTY_OUTPUT_ARCH           "iamcu:intel")
endif()

zephyr_sources(
  soc.c
  soc_config.c
  eoi.c
  power.c
  soc_power.S
  )
