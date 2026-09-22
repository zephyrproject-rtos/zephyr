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
    set(build_only)
    set(build_dir)

    get_target_property(build_only ${image} BUILD_ONLY)
    get_target_property(build_dir ${image} _EP_BINARY_DIR)

    if(DEFINED build_dir AND NOT build_only AND EXISTS ${build_dir}/zephyr/runners.yaml)
      set(board_target)
      sysbuild_get(board_target IMAGE ${image} VAR CONFIG_BOARD_TARGET KCONFIG)

      if(NOT board_target)
        continue()
      endif()

      string(REPLACE "/" "_" board_target ${board_target})
      string(REPLACE "." "_" board_target ${board_target})
      string(REPLACE "@" "_" board_target ${board_target})

      if(NOT board_target IN_LIST board_targets)
        list(APPEND board_targets ${board_target})
        set(board_merge_images_${board_target})
      endif()

      list(APPEND board_merge_images_${board_target} ${image})
    endif()
  endforeach()

  foreach(board_target ${board_targets})
    set(depends_list)
    set(input_list)
    set(extra_dependencies)

    foreach(image ${board_merge_images_${board_target}})
      # Only process images that create hex files
      set(output_hex)
      set(build_dir)
      set(application_image_dir)

      get_target_property(build_dir ${image} _EP_BINARY_DIR)
      yaml_load(FILE ${build_dir}/zephyr/runners.yaml NAME ${image}_runner_yaml)
      yaml_get(output_hex NAME ${image}_runner_yaml KEY config hex_file)
      sysbuild_get(application_image_dir IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)

      if(TARGET ${image}_extra_byproducts)
        list(APPEND depends_list ${image}_extra_byproducts)
      endif()

      if(application_image_dir AND EXISTS ${application_image_dir}/zephyr/.config)
        list(APPEND depends_list ${application_image_dir}/zephyr/.config)
      endif()

      if(IS_ABSOLUTE ${output_hex})
        list(APPEND input_list ${output_hex})
      else()
        list(APPEND input_list ${application_image_dir}/zephyr/${output_hex})
      endif()
    endforeach()

    get_property(extra_dependencies GLOBAL PROPERTY sysbuild_merged_hex_dependencies_${board_target})

    if(extra_dependencies)
      list(APPEND depends_list ${extra_dependencies})
    endif()

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
