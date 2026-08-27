# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

#[=======================================================================[.rst:
native_simulator_sb_extensions
##############################

Sysbuild extension commands for :zephyr:board:`native_sim` based targets.

These commands are used by the :file:`sysbuild.cmake` file of a board or application that builds
several images into a single native simulator executable. They are no-ops unless the board target
is a native simulator or :ref:`bsim <bsim boards>` based one.

#]=======================================================================]

#[=======================================================================[.rst:
.. cmake:signature:: native_simulator_set_final_executable(<final_image>)

   Add a build target which copies the executable produced by ``<final_image>`` to the top level,
   as :file:`zephyr/zephyr.exe`.

   ``<final_image>`` is expected to have been set to assemble other dependent images into itself
   if necessary, by calling :cmake:command:`native_simulator_set_child_images`.

   This allows other tools, like twister or the bsim test scripts, as well as users, to find this
   final executable in the same place as for non-sysbuild builds.

#]=======================================================================]
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

#[=======================================================================[.rst:
.. cmake:signature:: native_simulator_set_child_images(<final_image> <child_image>)

   Set ``<child_image>`` as a dependency of ``<final_image>``, and configure the final image to
   assemble the child image into its final executable.

#]=======================================================================]
function(native_simulator_set_child_images final_image child_image)
  if(("${BOARD}" MATCHES "native") OR ("${BOARD}" MATCHES "bsim"))
    add_dependencies(${final_image} ${child_image})

    set(CHILD_LIBRARY_PATH ${CMAKE_BINARY_DIR}/${child_image}/zephyr/zephyr.elf)
    set_property(TARGET ${final_image} APPEND_STRING PROPERTY CONFIG
      "CONFIG_NATIVE_SIMULATOR_EXTRA_IMAGE_PATHS=\"${CHILD_LIBRARY_PATH}\"\n"
    )
  endif()
endfunction()

#[=======================================================================[.rst:
.. cmake:signature:: native_simulator_set_primary_mcu_index(<image> [<image2> ...])

   Propagate the ``SB_CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX`` setting, if it is set, to the
   ``CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX`` of each given image.

#]=======================================================================]
function(native_simulator_set_primary_mcu_index)
  if(NOT ("${SB_CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX}" STREQUAL ""))
    foreach(arg IN LISTS ARGV)
      set_property(TARGET ${arg} APPEND_STRING PROPERTY CONFIG
        "CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX=${SB_CONFIG_NATIVE_SIMULATOR_PRIMARY_MCU_INDEX}\n"
      )
    endforeach()
  endif()
endfunction()
