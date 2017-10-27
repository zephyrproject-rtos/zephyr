set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-xtensa-le)


if(SOC_FAMILY)
  add_subdirectory(soc/${SOC_FAMILY})
else()
  add_subdirectory(soc/${SOC_PATH})
endif()

add_subdirectory(core)
