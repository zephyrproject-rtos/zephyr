# SPDX-License-Identifier: Apache-2.0

# '-mcmse' enables the generation of code for the Secure state of the ARMv8-M
# Security Extensions. This option is required when building a Secure firmware.

zephyr_compile_options_ifdef(CONFIG_ARM_SECURE_FIRMWARE $<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,cmse>>)
zephyr_compile_options_ifdef(CONFIG_ARM_SECURE_FIRMWARE $<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler,cmse>>)
zephyr_compile_options_ifdef(CONFIG_ARM_SECURE_FIRMWARE $<$<COMPILE_LANGUAGE:ASM>:$<TARGET_PROPERTY:asm,cmse>>)

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
# ARM_VENEERS_LIB_DIR may be set by the application CMakeLists.txt to point at
# a sibling image's build directory (e.g. in a sysbuild S+NS split).
if(CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS)
  if(DEFINED ARM_VENEERS_LIB_DIR)
    set(_veneers_dir ${ARM_VENEERS_LIB_DIR})
  else()
    set(_veneers_dir ${CMAKE_BINARY_DIR})
  endif()
  zephyr_link_libraries(${_veneers_dir}/${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME})
endif()

zephyr_library_sources_ifdef(CONFIG_ARM_SAU sau.c)

# secure_call_entries.c is a generated file whose GENERATED property is set in
# the root CMakeLists.txt directory scope (via add_custom_command OUTPUT).
# It cannot be added here via target_sources because source file properties are
# directory-scoped; the GENERATED flag would be invisible in this subdirectory.
# The root CMakeLists.txt adds it to the zephyr library after the custom command.
