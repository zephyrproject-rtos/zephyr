# Put functions and data in their own binary sections so that ld can
# garbage collect them
zephyr_cc_option(
  -ffunction-sections
  -fdata-sections
  )

add_subdirectory(soc)
add_subdirectory(core)

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-littleriscv)
