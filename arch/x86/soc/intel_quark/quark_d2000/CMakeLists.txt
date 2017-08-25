
zephyr_library_include_directories(PRIVATE ${PROJECT_SOURCE_DIR}/drivers)

if(CONFIG_SOC_QUARK_D2000)
  target_compile_definitions(zephyr PUBLIC
    QM_LAKEMONT
    -DSOC_SERIES=quark_d2000
    )
endif()

target_cc_option(zephyr PUBLIC -march=lakemont -mtune=lakemont -msoft-float)

if(CONFIG_X86_IAMCU)
  set_property(GLOBAL APPEND PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__IAMCU)
  set_property(GLOBAL        PROPERTY PROPERTY_OUTPUT_FORMAT         "elf32-iamcu")
  set_property(GLOBAL        PROPERTY PROPERTY_OUTPUT_ARCH           "iamcu:intel")
endif()
