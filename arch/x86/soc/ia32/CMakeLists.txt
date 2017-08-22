zephyr_library()
zephyr_library_include_directories(PRIVATE ${PROJECT_SOURCE_DIR}/drivers)

target_cc_option(zephyr PUBLIC -march=pentium)

if(CONFIG_X86_IAMCU)
  set_property(GLOBAL APPEND PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__IAMCU)
  set_property(GLOBAL        PROPERTY PROPERTY_OUTPUT_FORMAT         "elf32-iamcu")
  set_property(GLOBAL        PROPERTY PROPERTY_OUTPUT_ARCH           "iamcu:intel")
  target_cc_option(zephyr PUBLIC -msoft-float)
endif()

zephyr_library_sources(soc.c)
