
zephyr_library_include_directories(PRIVATE ${PROJECT_SOURCE_DIR}/drivers)

if(CONFIG_SOC_QUARK_SE_C1000)
  target_compile_definitions(zephyr PUBLIC
    QM_LAKEMONT
    -DSOC_SERIES=quark_se
    )
endif()

target_cc_option(zephyr PUBLIC -march=lakemont -mtune=lakemont -msoft-float)

if(CONFIG_X86_IAMCU)
  set_property(GLOBAL APPEND PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__IAMCU)
  set_property(GLOBAL        PROPERTY PROPERTY_OUTPUT_FORMAT         "elf32-iamcu")
  set_property(GLOBAL        PROPERTY PROPERTY_OUTPUT_ARCH           "iamcu:intel")
endif()

target_sources(zephyr PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/soc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/soc_config.c
  ${CMAKE_CURRENT_SOURCE_DIR}/eoi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/power.c
  ${CMAKE_CURRENT_SOURCE_DIR}/soc_power.S
  )
