# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Usage:
#   native_simulator_set_final_executable(<final_image>)
#
# When building for a native_simulator based target (including bsim targets),
# this function adds an extra build target which will copy the executable produced by
# `<final_image>` to the top level, as zephyr/zephyr.exe
#
# This final image is expected to have been set to assemble other dependent images into
# itself if necessary, by calling native_simulator_set_child_images()
# This will allow other tools, like twister, or the bsim test scripts, as well as users to find
# this final executable in the same place as for non-sysbuild builds.
#
function(native_simulator_set_final_executable final_image)
  if(("${BOARD}" MATCHES "native") OR ("${BOARD}" MATCHES "bsim"))
    add_custom_target(final_executable
      ALL
      COMMAND
      ${CMAKE_COMMAND} -E copy
      ${CMAKE_BINARY_DIR}/${final_image}/zephyr/zephyr.exe
      ${CMAKE_BINARY_DIR}/zephyr/zephyr.exe
      DEPENDS ${final_image}
    )
  endif()
endfunction()

# Usage:
#   native_simulator_set_child_images(<final_image> <child_image>)
#
# When building for a native_simulator based target (including bsim targets),
# this function sets a `<child_image>` as dependencies of `<final_image>`
# and configures the final image to assemble the child images into its final executable.
#
function(native_simulator_set_child_images final_image child_image)
  if(("${BOARD}" MATCHES "native") OR ("${BOARD}" MATCHES "bsim"))
    add_dependencies(${final_image} ${child_image})

    set(CHILD_LIBRARY_PATH ${CMAKE_BINARY_DIR}/${child_image}/zephyr/zephyr.elf)
    set_property(TARGET ${final_image} APPEND_STRING PROPERTY CONFIG
      "CONFIG_NATIVE_SIMULATOR_EXTRA_IMAGE_PATHS=\"${CHILD_LIBRARY_PATH}\"\n"
    )
  endif()
endfunction()

# Usage:
#   native_simulator_set_primary_mcu_index(<image> [<image2> ...])
#
# Propagate the SB_CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX setting,
# if it is set, to each given image CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX
#
function(native_simulator_set_primary_mcu_index)
  if (NOT ("${SB_CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX}" STREQUAL ""))
    foreach(arg IN LISTS ARGV)
      set_property(TARGET ${arg} APPEND_STRING PROPERTY CONFIG
        "CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX=${SB_CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX}\n"
      )
    endforeach()
  endif()
endfunction()
