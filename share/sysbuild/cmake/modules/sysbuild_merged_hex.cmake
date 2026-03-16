#
# Copyright (c) 2024-2026 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

# This module is responsible for creating merged hex files, one per unique board target. Hex
# files are merged using replace overlap mode from the images in ascending flash priority
# order (a later flashed application will overwrite data from previous images if both contain
# data in the same area)
function(create_merged_hex_files)
  set(board_targets)
  sysbuild_images_order(sorted_flash_images FLASH IMAGES ${IMAGES})

  # Parse each image to get the board target, normalise it and create a list of every image per
  # unique board target
  foreach(image ${sorted_flash_images})
    set(board_target)
    sysbuild_get(board_target IMAGE ${image} VAR CONFIG_BOARD_TARGET KCONFIG)
    string(REPLACE "/" "_" board_target ${board_target})
    string(REPLACE "." "_" board_target ${board_target})
    string(REPLACE "@" "_" board_target ${board_target})

    if(NOT board_target IN_LIST board_targets)
      list(APPEND board_targets ${board_target})
      set(board_merge_images_${board_target})
    endif()

    list(APPEND board_merge_images_${board_target} ${image})
  endforeach()

  foreach(board_target ${board_targets})
    set(depends_list)
    set(input_list)

    foreach(image ${board_merge_images_${board_target}})
      # Only process images that create hex files
      set(output_hex)
      sysbuild_get(output_hex IMAGE ${image} VAR CONFIG_BUILD_OUTPUT_HEX KCONFIG)

      if(output_hex)
        # The order of files to use are (starting with highest priority):
        # * <BYPRODUCT_KERNEL_SIGNED_CONFIRMED_HEX_NAME>
        # * <BYPRODUCT_KERNEL_SIGNED_HEX_NAME>
        # * <CONFIG_KERNEL_BIN_NAME>.hex
        #
        # This ensures that if the image has a signing script, that the signed image is used, the
        # confirmed version should be used if MCUboot is used in directXIP mode so has the most
        # priority. If there is no byproduct signed file, then the normal zephyr hex file is used
        set(byproduct_signed_confirmed_hex)
        set(byproduct_signed_hex)
        set(kernel_name)
        set(application_image_dir)
        sysbuild_get(byproduct_signed_confirmed_hex IMAGE ${image} VAR BYPRODUCT_KERNEL_SIGNED_CONFIRMED_HEX_NAME CACHE)
        sysbuild_get(byproduct_signed_hex IMAGE ${image} VAR BYPRODUCT_KERNEL_SIGNED_HEX_NAME CACHE)
        sysbuild_get(kernel_name IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
        sysbuild_get(application_image_dir IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
        list(APPEND depends_list ${image}_extra_byproducts ${application_image_dir}/zephyr/.config)

        if(byproduct_signed_confirmed_hex)
          list(APPEND input_list ${byproduct_signed_confirmed_hex})
        elseif(byproduct_signed_hex)
          list(APPEND input_list ${byproduct_signed_hex})
        else()
          list(APPEND input_list ${application_image_dir}/zephyr/${kernel_name}.hex)
        endif()
      endif()
    endforeach()

    # Use the mergehex python file to merge the hex file in flash-order with replace mode
    add_custom_command(
      OUTPUT
      ${CMAKE_BINARY_DIR}/merged_${board_target}.hex
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/build/mergehex.py
      -o ${CMAKE_BINARY_DIR}/merged_${board_target}.hex
      --overlap replace
      ${input_list}
      DEPENDS
      ${depends_list};${input_list}
      WORKING_DIRECTORY
      ${CMAKE_BINARY_DIR}
    )

    add_custom_target(
      merged_${board_target}_target
      ALL DEPENDS
      ${CMAKE_BINARY_DIR}/merged_${board_target}.hex
    )
  endforeach()
endfunction()

if(SB_CONFIG_MERGED_HEX_FILES)
  create_merged_hex_files()
endif()
