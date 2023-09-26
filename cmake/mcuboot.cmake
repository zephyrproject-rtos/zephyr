# Copyright (c) 2020-2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# This file includes extra build system logic that is enabled when
# CONFIG_BOOTLOADER_MCUBOOT=y.
#
# It builds signed binaries using imgtool as a post-processing step
# after zephyr/zephyr.elf is created in the build directory.
#
# Since this file is brought in via include(), we do the work in a
# function to avoid polluting the top-level scope.

function(zephyr_runner_file type path)
  # Property magic which makes west flash choose the signed build
  # output of a given type.
  set_target_properties(runners_yaml_props_target PROPERTIES "${type}_file" "${path}")
endfunction()

function(zephyr_mcuboot_tasks)
  set(keyfile "${CONFIG_MCUBOOT_SIGNATURE_KEY_FILE}")
  set(keyfile_enc "${CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE}")

  if(NOT "${CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE}")
    # Check for misconfiguration.
    if("${keyfile}" STREQUAL "")
      # No signature key file, no signed binaries. No error, though:
      # this is the documented behavior.
      message(WARNING "Neither CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE or "
                      "CONFIG_MCUBOOT_SIGNATURE_KEY_FILE are set, the generated build will not be "
                      "bootable by MCUboot unless it is signed manually/externally.")
      return()
    endif()
  endif()

  if(NOT WEST)
    # This feature requires west.
    message(FATAL_ERROR "Can't sign images for MCUboot: west not found. To fix, install west and ensure it's on PATH.")
  endif()

  foreach(file keyfile keyfile_enc)
    if(NOT "${${file}}" STREQUAL "")
      if(NOT IS_ABSOLUTE "${${file}}")
        # Relative paths are relative to 'west topdir'.
        set(${file} "${WEST_TOPDIR}/${${file}}")
      endif()

      if(NOT EXISTS "${${file}}" AND NOT "${CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE}")
        message(FATAL_ERROR "west sign can't find file ${${file}} (Note: Relative paths are relative to the west workspace topdir \"${WEST_TOPDIR}\")")
      elseif(NOT (CONFIG_BUILD_OUTPUT_BIN OR CONFIG_BUILD_OUTPUT_HEX))
        message(FATAL_ERROR "Can't sign images for MCUboot: Neither CONFIG_BUILD_OUTPUT_BIN nor CONFIG_BUILD_OUTPUT_HEX is enabled, so there's nothing to sign.")
      endif()
    endif()
  endforeach()

  # Find imgtool. Even though west is installed, imgtool might not be.
  # The user may also have a custom manifest which doesn't include
  # MCUboot.
  #
  # Therefore, go with an explicitly installed imgtool first, falling
  # back on mcuboot/scripts/imgtool.py.
  if(IMGTOOL)
    set(imgtool_path "${IMGTOOL}")
  elseif(DEFINED ZEPHYR_MCUBOOT_MODULE_DIR)
    set(IMGTOOL_PY "${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/imgtool.py")
    if(EXISTS "${IMGTOOL_PY}")
      set(imgtool_path "${IMGTOOL_PY}")
    endif()
  endif()

  # No imgtool, no signed binaries.
  if(NOT DEFINED imgtool_path)
    message(FATAL_ERROR "Can't sign images for MCUboot: can't find imgtool. To fix, install imgtool with pip3, or add the mcuboot repository to the west manifest and ensure it has a scripts/imgtool.py file.")
    return()
  endif()

  # Basic 'west sign' command and output format independent arguments.
  separate_arguments(west_sign_extra UNIX_COMMAND ${CONFIG_MCUBOOT_CMAKE_WEST_SIGN_PARAMS})
  set(west_sign ${WEST} sign ${west_sign_extra}
    --tool imgtool
    --tool-path "${imgtool_path}"
    --build-dir "${APPLICATION_BINARY_DIR}")

  # Arguments to imgtool.
  if(NOT CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS STREQUAL "")
    # Separate extra arguments into the proper format for adding to
    # extra_post_build_commands.
    #
    # Use UNIX_COMMAND syntax for uniform results across host
    # platforms.
    separate_arguments(imgtool_extra UNIX_COMMAND ${CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS})
  else()
    set(imgtool_extra)
  endif()

  if(NOT "${keyfile}" STREQUAL "")
    set(imgtool_extra --key "${keyfile}" ${imgtool_extra})
  endif()

  set(imgtool_args -- ${imgtool_extra})

  # Extensionless prefix of any output file.
  set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME})

  # List of additional build byproducts.
  set(byproducts)

  # 'west sign' arguments for confirmed, unconfirmed and encrypted images.
  set(unconfirmed_args)
  set(confirmed_args)
  set(encrypted_args)

  # Set up .bin outputs.
  if(CONFIG_BUILD_OUTPUT_BIN)
    list(APPEND unconfirmed_args --bin --sbin ${output}.signed.bin)
    list(APPEND byproducts ${output}.signed.bin)
    zephyr_runner_file(bin ${output}.signed.bin)
    set(BYPRODUCT_KERNEL_SIGNED_BIN_NAME "${output}.signed.bin"
        CACHE FILEPATH "Signed kernel bin file" FORCE
    )

    if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE)
      list(APPEND confirmed_args --bin --sbin ${output}.signed.confirmed.bin)
      list(APPEND byproducts ${output}.signed.confirmed.bin)
      set(BYPRODUCT_KERNEL_SIGNED_CONFIRMED_BIN_NAME "${output}.signed.confirmed.bin"
          CACHE FILEPATH "Signed and confirmed kernel bin file" FORCE
      )
    endif()

    if(NOT "${keyfile_enc}" STREQUAL "")
      list(APPEND encrypted_args --bin --sbin ${output}.signed.encrypted.bin)
      list(APPEND byproducts ${output}.signed.encrypted.bin)
      set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_BIN_NAME "${output}.signed.encrypted.bin"
          CACHE FILEPATH "Signed and encrypted kernel bin file" FORCE
      )
    endif()
  endif()

  # Set up .hex outputs.
  if(CONFIG_BUILD_OUTPUT_HEX)
    list(APPEND unconfirmed_args --hex --shex ${output}.signed.hex)
    list(APPEND byproducts ${output}.signed.hex)
    zephyr_runner_file(hex ${output}.signed.hex)
    set(BYPRODUCT_KERNEL_SIGNED_HEX_NAME "${output}.signed.hex"
        CACHE FILEPATH "Signed kernel hex file" FORCE
    )

    if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE)
      list(APPEND confirmed_args --hex --shex ${output}.signed.confirmed.hex)
      list(APPEND byproducts ${output}.signed.confirmed.hex)
      set(BYPRODUCT_KERNEL_SIGNED_CONFIRMED_HEX_NAME "${output}.signed.confirmed.hex"
          CACHE FILEPATH "Signed and confirmed kernel hex file" FORCE
      )
    endif()

    if(NOT "${keyfile_enc}" STREQUAL "")
      list(APPEND encrypted_args --hex --shex ${output}.signed.encrypted.hex)
      list(APPEND byproducts ${output}.signed.encrypted.hex)
      set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_HEX_NAME "${output}.signed.encrypted.hex"
          CACHE FILEPATH "Signed and encrypted kernel hex file" FORCE
      )
    endif()
  endif()

  # Add the west sign calls and their byproducts to the post-processing
  # steps for zephyr.elf.
  #
  # CMake guarantees that multiple COMMANDs given to
  # add_custom_command() are run in order, so adding the 'west sign'
  # calls to the "extra_post_build_commands" property ensures they run
  # after the commands which generate the unsigned versions.
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
    ${west_sign} ${unconfirmed_args} ${imgtool_args})
  if(confirmed_args)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${west_sign} ${confirmed_args} ${imgtool_args} --pad --confirm)
  endif()
  if(encrypted_args)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${west_sign} ${encrypted_args} ${imgtool_args} --encrypt "${keyfile_enc}")
  endif()
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${byproducts})
endfunction()

zephyr_mcuboot_tasks()
