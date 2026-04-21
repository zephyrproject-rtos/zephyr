# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME})

function(zephyr_runner_file type path)
  # Property magic which makes west flash choose the signed build output of a given type.
  set_target_properties(runners_yaml_props_target PROPERTIES "${type}_file" "${path}")
endfunction()

zephyr_linker_sources(ROM_START SORT_KEY 0x0header sbt/sbt_sla_header.ld)
zephyr_linker_sources(SECTIONS sbt/sbt_sla_symbols.ld)

set(signing_tool sign_app)

find_program(signing_tool_path ${signing_tool})

if(signing_tool_path STREQUAL signing_tool_path-NOTFOUND)
  message(WARNING "Signing Image tool (\"${signing_tool}\") is not available so signed image "
                  "will not be generated. You won't be able to run application on the board. "
                  "Refer to board documentation for more information.")
else()
  message(STATUS "Signing Image tool (\"${signing_tool}\") is available at "
                 "\"${signing_tool_path}\".")

  if(CONFIG_MAX32_SECURE_SOC_KEY_PATH STREQUAL "")
    message(WARNING "CONFIG_MAX32_SECURE_SOC_KEY_PATH is not set. Because of this, signed image "
                    "could not be generated.")
  else()
    message(STATUS "\"${CONFIG_MAX32_SECURE_SOC_KEY_PATH}\" will be used to sign the image.")
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    # Remove the .sig section from the ELF file before signing.
    COMMAND ${CMAKE_OBJCOPY} -O binary --remove-section=.sig
    ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} ${output}_removed_sig.bin

    # Sign the binary using the signing tool. The tool takes the binary without the .sig section,
    # generates a signature.
    COMMAND ${signing_tool} -c ${CONFIG_SOC}
    ca=${output}_removed_sig.bin
    sca=${output}.sbin
    key_file=${CONFIG_MAX32_SECURE_SOC_KEY_PATH}

    # Add the .sig section back to the ELF file after signing.
    COMMAND ${CMAKE_OBJCOPY} -O elf32-littlearm ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}
    --update-section .sig=${ZEPHYR_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.sig ${output}.signed.elf
  )

  if(CONFIG_BUILD_OUTPUT_HEX)
    zephyr_runner_file(hex ${output}.signed.hex)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      # --gap-fill=${CONFIG_BUILD_GAP_FILL_PATTERN} fills NOBITS sections (eg. tbss) to their full memory size.
      # This prevents signature mismatch between signed ELF and flashed HEX during secure boot.
      COMMAND ${CMAKE_OBJCOPY} -O ihex --gap-fill=${CONFIG_BUILD_GAP_FILL_PATTERN} ${output}.signed.elf ${output}.signed.hex
    )
  endif()

  if(CONFIG_BUILD_OUTPUT_BIN)
    zephyr_runner_file(bin ${output}.signed.bin)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      # --gap-fill=${CONFIG_BUILD_GAP_FILL_PATTERN} fills NOBITS sections (eg. tbss) to their full memory size.
      # This prevents signature mismatch between signed ELF and flashed BIN during secure boot.
      COMMAND ${CMAKE_OBJCOPY} -O binary --gap-fill=${CONFIG_BUILD_GAP_FILL_PATTERN} ${output}.signed.elf ${output}.signed.bin
    )
  endif()
endif()
