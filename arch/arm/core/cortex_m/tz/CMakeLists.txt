# '-mcmse' enables the generation of code for the Secure state of the ARMv8-M
# Security Extensions. This option is required when building a Secure firmware.
zephyr_compile_options(-mcmse)

if(CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS)

# --out-implib and --cmse-implib instruct the linker to produce
# an import library that consists of a relocatable file containing
# only a symbol table with the entry veneers. The library may be used
# when building a Non-Secure image which shall have access to Secure
# Entry functions.
  zephyr_ld_options(
    ${LINKERFLAGPREFIX},--out-implib=${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME}
  )

  zephyr_ld_options(
    ${LINKERFLAGPREFIX},--cmse-implib
  )
endif()

zephyr_sources(arm_core_tz.c)
