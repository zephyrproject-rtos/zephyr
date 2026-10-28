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
# - a fatal error when the board provides a requested setup neither with
#   files nor, for a setup naming what the devicetree shows when it is
#   provided, with its own devicetree; or when the files it has do not
#   make the devicetree show it

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

  yaml_get(compatible NAME vane_${vane} KEY compatible)
  yaml_get(chosen NAME vane_${vane} KEY chosen)
  yaml_get(node NAME vane_${vane} KEY node)
  if(vane_dts OR vane_conf)
    set(vane_${vane}_realized TRUE)
  elseif(NOT (compatible OR chosen OR node))
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

# Once the devicetree is processed, check that it shows each setup that names
# what it shows when provided: through the files found, or, with none, the
# board's own devicetree.
function(vanes_check)
  foreach(vane IN LISTS VANES)
    yaml_get(compatible NAME vane_${vane} KEY compatible)
    yaml_get(chosen NAME vane_${vane} KEY chosen)
    yaml_get(node NAME vane_${vane} KEY node)
    yaml_get(property NAME vane_${vane} KEY property)
    set(provided FALSE)
    if(compatible)
      set(shows "enabled '${compatible}' node")
      dt_comp_path(paths COMPATIBLE ${compatible})
      foreach(path IN LISTS paths)
        dt_node_has_status(okay PATH ${path} STATUS okay)
        if(okay)
          set(provided TRUE)
        endif()
      endforeach()
    elseif(chosen)
      set(shows "enabled '${chosen}' chosen node")
      dt_has_chosen(has_chosen PROPERTY ${chosen})
      if(has_chosen)
        dt_chosen(path PROPERTY ${chosen})
        dt_node_has_status(provided PATH ${path} STATUS okay)
      endif()
    elseif(node)
      set(shows "'${node}' node")
      dt_node_exists(provided PATH ${node})
      if(provided AND property)
        set(shows "'${node}' node with a '${property}' property")
        dt_prop(value PATH ${node} PROPERTY ${property})
        if(NOT DEFINED value)
          set(provided FALSE)
        endif()
      endif()
    else()
      continue()
    endif()
    if(provided)
      continue()
    endif()
    if(vane_${vane}_realized)
      message(FATAL_ERROR
        "The vanes/${vane}/ files of ${BOARD}/${BOARD_QUALIFIERS} do not provide the "
        "'${vane}' test setup: the devicetree has no ${shows}."
      )
    endif()
    message(FATAL_ERROR
      "Board ${BOARD}/${BOARD_QUALIFIERS} does not provide the '${vane}' test setup: "
      "no vanes/${vane}/ files for it in its board or SoC directory, and its own "
      "devicetree has no ${shows}."
    )
  endforeach()
endfunction()

# Not in script mode: there is no directory to defer the call to, and no
# build to fail.
if(NOT CMAKE_SCRIPT_MODE_FILE)
  cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL vanes_check)
endif()
