# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Vanes: board-owned test metadata realizing a named test setup.
#
# A test names the setups it needs; twister, or a developer, requests them
# with VANES. For each setup, the devicetree overlays and Kconfig fragments
# a board or a SoC keeps in vanes/<setup>/ under its own directory are
# applied, named like the boards/ and socs/ configuration files of an
# application. The setups themselves are defined in tests/vanes/.
#
# Outcome:
# - EXTRA_DTC_OVERLAY_FILE and EXTRA_CONF_FILE extended with the files found
# - a fatal error when a board provides nothing for a requested setup, unless
#   the setup states a filter and so can hold from the board's devicetree
#   alone, or when the setup names a devicetree node the build does not
#   enable

include_guard(GLOBAL)

include(extensions)

# The setups apply to the main application image only under sysbuild.
zephyr_get(VANES SYSBUILD LOCAL)

if(NOT VANES)
  return()
endif()

set(VANE_ROOT ${ZEPHYR_BASE}/tests/vanes)
zephyr_create_scope(vanes)

string(REGEX MATCH "^[^/]*" vane_soc "${BOARD_QUALIFIERS}")
foreach(vane IN LISTS VANES)
  if(NOT EXISTS ${VANE_ROOT}/${vane}.yml)
    message(FATAL_ERROR "No test setup named '${vane}' in ${VANE_ROOT}")
  endif()
  yaml_load(FILE ${VANE_ROOT}/${vane}.yml NAME vane_${vane})

  set(vane_dts)
  set(vane_conf)
  foreach(soc_dir IN LISTS SOC_${vane_soc}_DIRECTORIES)
    if(IS_DIRECTORY ${soc_dir}/vanes/${vane})
      zephyr_file(CONF_FILES ${soc_dir}/vanes/${vane} DTS vane_dts KCONF vane_conf QUALIFIERS)
    endif()
  endforeach()
  foreach(board_dir IN LISTS BOARD_DIRECTORIES)
    if(IS_DIRECTORY ${board_dir}/vanes/${vane})
      zephyr_file(CONF_FILES ${board_dir}/vanes/${vane} DTS vane_dts KCONF vane_conf)
    endif()
  endforeach()

  yaml_get(filter NAME vane_${vane} KEY filter)
  if(NOT vane_dts AND NOT vane_conf AND NOT filter)
    message(FATAL_ERROR
      "Board ${BOARD}/${BOARD_QUALIFIERS} does not provide the '${vane}' test setup: "
      "no vanes/${vane}/ files for it in its board or SoC directory."
    )
  endif()
  foreach(file IN LISTS vane_dts)
    zephyr_set(EXTRA_DTC_OVERLAY_FILE ${file} SCOPE vanes APPEND)
  endforeach()
  foreach(file IN LISTS vane_conf)
    zephyr_set(EXTRA_CONF_FILE ${file} SCOPE vanes APPEND)
  endforeach()
endforeach()

# Once the devicetree is processed, check that it enables the node the setup
# names, if it names one.
function(vanes_check)
  foreach(vane IN LISTS VANES)
    yaml_get(compatible NAME vane_${vane} KEY compatible)
    if(NOT compatible)
      continue()
    endif()
    dt_comp_path(paths COMPATIBLE ${compatible})
    set(enabled FALSE)
    foreach(path IN LISTS paths)
      dt_node_has_status(okay PATH ${path} STATUS okay)
      if(okay)
        set(enabled TRUE)
      endif()
    endforeach()
    if(NOT enabled)
      message(FATAL_ERROR
        "The '${vane}' test setup of ${BOARD}/${BOARD_QUALIFIERS} does not enable a "
        "'${compatible}' node, which the setup requires."
      )
    endif()
  endforeach()
endfunction()

# Not in script mode, where twister evaluates its filters: there is no
# directory to defer the call to, and no build to fail.
if(NOT CMAKE_SCRIPT_MODE_FILE)
  cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL vanes_check)
endif()
