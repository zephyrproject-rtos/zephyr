set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-tradlittlemips)

set_property(GLOBAL PROPERTY E_KERNEL_ENTRY  -e${CONFIG_KERNEL_ENTRY})

add_subdirectory(soc)
add_subdirectory(core)
