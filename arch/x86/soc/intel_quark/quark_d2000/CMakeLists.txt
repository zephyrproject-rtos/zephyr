zephyr_library_include_directories(${PROJECT_SOURCE_DIR}/drivers)

zephyr_compile_definitions_ifdef(
  CONFIG_SOC_QUARK_D2000
  QM_LAKEMONT
  SOC_SERIES=quark_d2000
  )

zephyr_cc_option(-march=lakemont -mtune=lakemont -msoft-float)

if(CONFIG_X86_IAMCU)
  set_property(GLOBAL APPEND PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__IAMCU)
  set_property(GLOBAL        PROPERTY PROPERTY_OUTPUT_FORMAT         "elf32-iamcu")
  set_property(GLOBAL        PROPERTY PROPERTY_OUTPUT_ARCH           "iamcu:intel")
endif()
