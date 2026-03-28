# Copyright (c) 2020-2023 Nordic Semiconductor ASA
# Copyright (c) 2024 Cypress Semiconductor Corporation
# SPDX-License-Identifier: Apache-2.0

# This file includes extra build system logic that is enabled when
# CONFIG_BOOTLOADER_MCUBOOT=y.
#
# It builds signed binaries using cysecuretools as a post-processing step
# after zephyr/zephyr.elf is created in the build directory.

function(zephyr_runner_file type path)
  # Property magic which makes west flash choose the signed build
  # output of a given type.
  set_target_properties(runners_yaml_props_target PROPERTIES "${type}_file" "${path}")
endfunction()

function(zephyr_mcuboot_tasks)
  # Extensionless prefix of any output file.
  set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME})

  cmake_path(SET keyfile "${CONFIG_MCUBOOT_SIGNATURE_KEY_FILE}")
  cmake_path(SET keyfile_enc "${CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE}")

  set(encrypted_args)
  set(confirmed_args)

  # Calculate flash address (SAHB/CBUS)
  math(EXPR flash_addr_sahb_offset
       "${CONFIG_CYW20829_FLASH_SAHB_ADDR} + ${CONFIG_FLASH_LOAD_OFFSET}"
       OUTPUT_FORMAT HEXADECIMAL
    )

  math(EXPR flash_addr_sbus_offset
       "${CONFIG_CYW20829_FLASH_CBUS_ADDR} + ${CONFIG_FLASH_LOAD_OFFSET}"
       OUTPUT_FORMAT HEXADECIMAL
    )

  # Check for misconfiguration.
  if((NOT "${CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE}") AND ("${keyfile}" STREQUAL ""))
      message(WARNING "Neither CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE nor"
                      "CONFIG_MCUBOOT_SIGNATURE_KEY_FILE are set, the generated build will not be"
                      "bootable by MCUboot unless it is signed manually/externally.")
      return()
  endif()

  foreach(file keyfile keyfile_enc)
    if(NOT "${${file}}" STREQUAL "")
      if(NOT IS_ABSOLUTE "${${file}}")
        find_file(
          temp_file
          NAMES
            "${${file}}"
          PATHS
            "${APPLICATION_SOURCE_DIR}"
            "${WEST_TOPDIR}"
          NO_DEFAULT_PATH
          )

          if(NOT "${temp_file}" STREQUAL "temp_file-NOTFOUND")
            set(${file} "${temp_file}")
          endif()
      endif()

      if((NOT IS_ABSOLUTE "${${file}}" OR NOT EXISTS "${${file}}") AND NOT "${CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE}")
        message(FATAL_ERROR "Can't find file \"${${file}}\" "
                            "(Note: Relative paths are searched through"
                            "APPLICATION_SOURCE_DIR=\"${APPLICATION_SOURCE_DIR}\" "
                            "and WEST_TOPDIR=\"${WEST_TOPDIR}\")")
      elseif(NOT (CONFIG_BUILD_OUTPUT_BIN OR CONFIG_BUILD_OUTPUT_HEX))
        message(FATAL_ERROR "Can't sign images for MCUboot: Neither CONFIG_BUILD_OUTPUT_BIN nor"
                            "CONFIG_BUILD_OUTPUT_HEX is enabled, so there's nothing to sign.")
      endif()
    endif()
  endforeach()

  # Basic 'cysecuretools' command and output format independent arguments.
  set(cysecuretools_cmd ${CYSECURETOOLS} -q -t cyw20829 -p ${CYSECURETOOLS_POLICY})

  # sign-image arguments.
  set(sign_image_cmd_args sign-image
      --image-format mcuboot_user_app
      --image "${MERGED_FILE}"
      --slot-size ${CONFIG_FLASH_LOAD_SIZE}
      --align 1
      --image-id 0
      --hex-addr ${flash_addr_sahb_offset}
      --app-addr ${flash_addr_sbus_offset}
      -v "${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}")

  # Extra arguments from CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS.
  if(NOT CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS STREQUAL "")
    # Separate extra arguments into the proper format for adding to
    # extra_post_build_commands.
    #
    # Use UNIX_COMMAND syntax for uniform results across host
    # platforms.
    separate_arguments(cysecuretools_extra_args UNIX_COMMAND
                       ${CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS})
  else()
    set(cysecuretools_extra_args)
  endif()

  if(NOT "${keyfile}" STREQUAL "")
    set(sign_image_cmd_args ${sign_image_cmd_args} --key-path "${keyfile}")
  endif()

  if(NOT "${keyfile_enc}" STREQUAL "")
    set(encrypted_args --encrypt --enckey "${keyfile_enc}")
  endif()

  # Use overwrite-only instead of swap upgrades.
  if(CONFIG_MCUBOOT_IMGTOOL_OVERWRITE_ONLY)
    set(sign_image_cmd_args ${sign_image_cmd_args} --overwrite-only --align 1)
  endif()

  if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE)
    list(APPEND confirmed_args --pad --confirm)
  endif()

  # List of additional build byproducts.
  set(byproducts)
  set(bin2hex_cmd_args_signed)
  set(bin2hex_cmd_args_confirmed)
  set(bin2hex_cmd_args_encrypted)

  # Set up .bin outputs.
  if(CONFIG_BUILD_OUTPUT_BIN)
    list(APPEND byproducts ${output}.signed.bin)
    zephyr_runner_file(bin ${output}.signed.bin)
    set(BYPRODUCT_KERNEL_SIGNED_BIN_NAME "${output}.signed.bin"
        CACHE FILEPATH "Signed kernel bin file" FORCE
   )
  endif()

  if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE)
    list(APPEND byproducts ${output}.signed.confirmed.bin)
    set(BYPRODUCT_KERNEL_SIGNED_CONFIRMED_BIN_NAME "${output}.signed.confirmed.bin"
        CACHE FILEPATH "Signed and confirmed kernel bin file" FORCE
    )
  endif()

  if(NOT "${keyfile_enc}" STREQUAL "")
    list(APPEND byproducts ${output}.signed.encrypted.bin)
    set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_BIN_NAME "${output}.signed.encrypted.bin"
        CACHE FILEPATH "Signed and encrypted kernel bin file" FORCE
    )
  endif()

  # Set up .hex outputs.
  if(CONFIG_BUILD_OUTPUT_HEX)
    list(APPEND bin2hex_cmd_args_signed bin2hex
          --image ${output}.signed.bin --output ${output}.signed.hex
          --offset ${flash_addr_sahb_offset}
    )
    list(APPEND byproducts ${output}.signed.hex)
    zephyr_runner_file(hex ${output}.signed.hex)
    set(BYPRODUCT_KERNEL_SIGNED_HEX_NAME "${output}.signed.hex"
        CACHE FILEPATH "Signed kernel hex file" FORCE)

    if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE)
      list(APPEND bin2hex_cmd_args_confirmed bin2hex
            --image ${output}.signed.confirmed.bin --output ${output}.signed.confirmed.hex
            --offset ${flash_addr_sahb_offset}
      )
      list(APPEND byproducts ${output}.signed.confirmed.hex)
      set(BYPRODUCT_KERNEL_SIGNED_CONFIRMED_HEX_NAME "${output}.signed.confirmed.hex"
          CACHE FILEPATH "Signed and confirmed kernel hex file" FORCE
      )
    endif()

    if(NOT "${keyfile_enc}" STREQUAL "")
      list(APPEND bin2hex_cmd_args_encrypted bin2hex
           --image ${output}.signed.encrypted.bin --output ${output}.signed.encrypted.hex
           --offset ${flash_addr_sahb_offset}
      )
      list(APPEND byproducts ${output}.signed.encrypted.hex)
      set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_HEX_NAME "${output}.signed.encrypted.hex"
          CACHE FILEPATH "Signed and encrypted kernel hex file" FORCE
      )
    endif()
  endif()

  # Add the post-processing steps to generate
  # signed /signed.confirmed / signed.encrypted bin and hex files
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               ${cysecuretools_cmd}
               ${sign_image_cmd_args} --output ${output}.signed.bin
               ${bin2hex_cmd_args_signed} # bin to hex
               ${cysecuretools_extra_args}) # from CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS

  if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
                 ${cysecuretools_cmd}
                 ${sign_image_cmd_args} --output ${output}.signed.confirmed.bin
                 ${confirmed_args}
                 ${bin2hex_cmd_args_confirmed} # bin to hex
                 ${cysecuretools_extra_args}) # from CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS
  endif()

  if(NOT "${keyfile_enc}" STREQUAL "")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
                 ${cysecuretools_cmd}
                 ${sign_image_cmd_args} --output ${output}.signed.encrypted.bin
                 ${confirmed_args} ${encrypted_args}
                 ${bin2hex_cmd_args_encrypted} # bin to hex
                 ${cysecuretools_extra_args} # from CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS

                 COMMAND ${CMAKE_COMMAND} -E echo
                 "Generating encrypted files ${output}.signed.encrypted.hex/bin files"

                 COMMAND ${CMAKE_COMMAND} -E echo
                 \"Use 'west flash --hex-file ${output}.signed.encrypted.hex' to flash in primary
                 partition\")
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${byproducts})
endfunction()

if((NOT "${CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE}") OR (NOT "${CONFIG_MCUBOOT_SIGNATURE_KEY_FILE}" STREQUAL ""))
  zephyr_mcuboot_tasks()
endif()
