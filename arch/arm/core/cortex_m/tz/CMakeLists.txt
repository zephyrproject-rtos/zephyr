# SPDX-License-Identifier: Apache-2.0

# '-mcmse' enables the generation of code for the Secure state of the ARMv8-M
# Security Extensions. This option is required when building a Secure firmware.
zephyr_compile_options_ifdef(CONFIG_ARM_SECURE_FIRMWARE -mcmse)

if(CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS)

  # --out-implib and --cmse-implib instruct the linker to produce
  # an import library that consists of a relocatable file containing
  # only a symbol table with the entry veneers. The library may be used
  # when building a Non-Secure image which shall have access to Secure
  # Entry functions.
  zephyr_ld_options(
    ${LINKERFLAGPREFIX},--out-implib=${CMAKE_BINARY_DIR}/${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME}
  )

  zephyr_ld_options(
    ${LINKERFLAGPREFIX},--cmse-implib
  )

  # Indicate that the entry veneers library file is created during linking of this firmware.
  set_property(
    GLOBAL APPEND PROPERTY
    extra_post_build_byproducts
    ${CMAKE_BINARY_DIR}/${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME}
    )

  zephyr_linker_sources(SECTIONS SORT_KEY z_end secure_entry_functions.ld)
endif()

# Link the entry veneers library file with the Non-Secure Firmware that needs it.
zephyr_link_libraries_ifdef(CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS
  ${CMAKE_BINARY_DIR}/${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME}
  )
