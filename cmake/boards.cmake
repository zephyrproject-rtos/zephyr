# SPDX-License-Identifier: Apache-2.0

# List all architectures, export the list in list_var
function(list_archs list_var)

  FILE(GLOB arch_contents RELATIVE $ENV{ZEPHYR_BASE}/arch $ENV{ZEPHYR_BASE}/arch/*)
  set(_arch_list)
  foreach(f ${arch_contents})
    if ("${f}" STREQUAL "common")
      continue()
    endif()
    if (IS_DIRECTORY "$ENV{ZEPHYR_BASE}/arch/${f}")
      list(APPEND _arch_list "${f}")
    endif()
  endforeach()

  # Export the arch list to the parent scope
  set(${list_var} ${_arch_list} PARENT_SCOPE)
endfunction()

# List all boards for a particular arch, export the list in list_var
function(list_boards arch list_var)

  # Make sure we start with an empty list for each arch
  set(_boards_for_${arch} "")
  foreach(root ${BOARD_ROOT})
    set(board_arch_dir ${root}/boards/${arch})

    # Match the _defconfig files in the board directories to make sure we are
    # finding boards, e.g. qemu_xtensa/qemu_xtensa_defconfig
    file(GLOB_RECURSE defconfigs_for_${arch}
      RELATIVE ${board_arch_dir}
      ${board_arch_dir}/*_defconfig
      )

    # The above gives a list like
    # nrf51_blenano/nrf51_blenano_defconfig;nrf51_pca10028/nrf51_pca10028_defconfig
    # we construct a list of board names by removing both the _defconfig
    # suffix and the path.
    foreach(defconfig_path ${defconfigs_for_${arch}})
      get_filename_component(board ${defconfig_path} NAME)
      string(REPLACE "_defconfig" "" board "${board}")
      list(APPEND _boards_for_${arch} ${board})
    endforeach()

  endforeach()

  # Export the board list for this arch to the parent scope
  set(${list_var} ${_boards_for_${arch}} PARENT_SCOPE)
endfunction()

# Function to dump and optionally save to a file all architectures and boards
# Takes the board root as a semicolon-separated list in:
# BOARD_ROOT
# Additional arguments:
# file_out Filename to save the boards list. If not defined, stdout will be used
# indent Prepend an additional indendation string to each line
function(dump_all_boards file_out indent)

  set(arch_list)
  list_archs(arch_list)

  foreach(arch ${arch_list})
    list_boards(${arch} boards_for_${arch})
  endforeach()

  if(file_out)
    file(WRITE ${file_out} "")
  endif()

  foreach(arch ${arch_list})
    set(_arch_str "${indent}${arch}:")
    if(file_out)
      file(APPEND ${file_out} "${_arch_str}\n")
    else()
      message(${_arch_str})
    endif()
    foreach(board ${boards_for_${arch}})
      set(_board_str "${indent}  ${board}")
      if(file_out)
         file(APPEND ${file_out} "${_board_str}\n")
      else()
         message(${_board_str})
      endif()
    endforeach()
  endforeach()

endfunction()

if(CMAKE_SCRIPT_MODE_FILE AND NOT CMAKE_PARENT_LIST_FILE)
# If this file is invoked as a script directly with -P:
# cmake [options] -P board.cmake
# Note that CMAKE_PARENT_LIST_FILE not being set ensures that this present
# file is being invoked directly with -P, and not via an include directive from
# some other script

# The options available are:
# BOARD_ROOT_SPACE_SEPARATED: Space-separated board roots
# FILE_OUT: Set to a file path to save the boards to a file. If not defined the
#           the contents will be printed to stdout
if(NOT DEFINED ENV{ZEPHYR_BASE})
  message(FATAL_ERROR "ZEPHYR_BASE not set")
endif()

if (NOT BOARD_ROOT_SPACE_SEPARATED)
  message(FATAL_ERROR "BOARD_ROOT_SPACE_SEPARATED not defined")
endif()

string(REPLACE " " ";" BOARD_ROOT "${BOARD_ROOT_SPACE_SEPARATED}")

if (NOT FILE_OUT)
  set(FILE_OUT FALSE)
endif()

dump_all_boards(${FILE_OUT} "")

endif()

