# Copyright (c) 2024 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(mpu_regions.c)
zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_PM power.c)

# CAT1B family defines
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1 CY_USING_HAL)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_INFINEON_CAT1B COMPONENT_CAT1B)
zephyr_compile_definitions(COMPONENT_CM33)

# In MTB for APPTYPE == flash: -DFLASH_BOOT -DCY_PDL_FLASH_BOOT
zephyr_compile_definitions(FLASH_BOOT)
zephyr_compile_definitions(CY_PDL_FLASH_BOOT)

# Use custome linker script
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/soc/infineon/cat1b/cyw20829/linker.ld CACHE INTERNAL "")

# Get sram_bootstrap address and size
dt_nodelabel(sram_bootstrap NODELABEL "sram_bootstrap")
dt_reg_addr(bootstrap_dst_addr PATH ${sram_bootstrap})
dt_reg_size(bootstrap_size PATH ${sram_bootstrap})

# Calculate the place in flash
math(EXPR flash_addr_offset
  "${CONFIG_CYW20829_FLASH_SAHB_ADDR} + ${CONFIG_FLASH_LOAD_OFFSET} + ${CONFIG_ROM_START_OFFSET}"
  OUTPUT_FORMAT HEXADECIMAL
)
set(gen_app_header_args --flash_addr_offset ${flash_addr_offset})
set(app_signed_enc_path ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME})

if(CONFIG_INFINEON_SECURE_LCS OR CONFIG_BOOTLOADER_MCUBOOT)
  # Check cysecuretools
  find_program(CYSECURETOOLS cysecuretools)
  if(NOT CYSECURETOOLS)
    message(FATAL_ERROR "Can't find cysecuretools. To fix, install cysecuretools with pip3.")
  else()
    message("-- Found cysecuretools: ${CYSECURETOOLS}")
  endif()
endif()

if(CONFIG_INFINEON_SECURE_LCS)
  set(default_policy)
  set(default_policy_name)

  # Cysecuretools policy.
  if(NOT CONFIG_INFINEON_SECURE_POLICY)
    # Get default cysecuretools policy
    if(CONFIG_INFINEON_SECURE_LCS)
      message(INFO "CONFIG_INFINEON_SECURE_POLICY was not defined.")
      set(default_policy_name policy_secure.json)
    else()
      set(default_policy_name policy_no_secure.json)
    endif()
  endif()

  find_file(
    default_policy
    NAMES
        ${CONFIG_INFINEON_SECURE_POLICY}
        ${default_policy_name}
    PATHS
        ${APPLICATION_SOURCE_DIR}
        ${ZEPHYR_BASE}
    NO_DEFAULT_PATH
    )

  if(NOT default_policy)
      message(FATAL_ERROR "Can't find policy:${CONFIG_INFINEON_SECURE_POLICY}"
              "/${default_policy_name}"
              "Checked locations: ${APPLICATION_SOURCE_DIR}, ${ZEPHYR_BASE}")
  endif()

  set(cysecuretools_policy ${default_policy} CACHE PATH "cysecuretools policy")
  message("-- Using cysecuretools policy: ${cysecuretools_policy}")
endif()

if(CONFIG_INFINEON_SECURE_LCS)
  #
  # Addition postbuild action for SECURE LCS
  #
  set(gen_app_header_args ${gen_app_header_args} --secure_lcs True)
  set(app_signed_enc_path ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed)

  if(CONFIG_INFINEON_SMIF_ENCRYPTION)
    set(gen_app_header_args ${gen_app_header_args} --smif-config ${ZEPHYR_BINARY_DIR}/nonce-output.bin)
    set(enc_option --encrypt --nonce-output nonce-output.bin)
    set(app_signed_enc_path ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed.encrypted)
  endif()

  set(bin2hex_option bin2hex --image ${app_signed_enc_path}.bin --output ${app_signed_enc_path}.hex --offset 0x60000030)

  # Sign Zephyr L1 app in SECURE LCS
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${CYSECURETOOLS} -q -t cyw20829
      -p ${cysecuretools_policy} sign-image --image-format bootrom_next_app
      -i ${ZEPHYR_BINARY_DIR}/${KERNEL_BIN_NAME} -k 0 -o ${app_signed_enc_path}.bin
      --slot-size ${CONFIG_FLASH_LOAD_SIZE} --app-addr 0x08000030
      ${enc_option} ${bin2hex_option}
    )
endif()

# Generate platform specific header (TOC2, l1_desc, etc)
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
    ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/soc/infineon/cat1b/cyw20829/gen_app_header.py
    -p ${ZEPHYR_BINARY_DIR} -n ${KERNEL_NAME} ${gen_app_header_args}
    --bootstrap-size ${bootstrap_size}
    --bootstrap-dst-addr ${bootstrap_dst_addr}
  )

set(MERGED_FILE ${CMAKE_BINARY_DIR}/zephyr/zephyr_merged.hex)

# Merge platform specific header and zephyr image to a single binary.
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
    -o ${MERGED_FILE}
    ${app_signed_enc_path}.hex ${ZEPHYR_BINARY_DIR}/app_header.hex
  )

set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${MERGED_FILE})
