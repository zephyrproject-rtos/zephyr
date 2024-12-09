# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Validate shields and setup shields target.
#
# This module will validate the SHIELD argument.
#
# If a shield implementation is not found for one of the specified shields, an
# error will be raised and a list of valid shields will be printed.
#
# Outcome:
# The following variables will be defined when this module completes:
# - shield_conf_files: List of shield-specific Kconfig fragments
# - shield_dts_files : List of shield-specific devicetree files
# - SHIELD_AS_LIST   : A CMake list of shields created from the SHIELD variable.
# - SHIELD_DIRS      : A CMake list of directories which contain shield definitions
#
# The following targets will be defined when this CMake module completes:
# - shields: when invoked, a list of valid shields will be printed
#
# If the SHIELD variable is changed after this module completes,
# a warning will be printed.
#
# Optional variables:
# - BOARD_ROOT: CMake list of board roots containing board implementations
#
# Variables set by this module and not mentioned above are for internal
# use only, and may be removed, renamed, or re-purposed without prior notice.

include_guard(GLOBAL)

include(extensions)

# Check that SHIELD has not changed.
zephyr_check_cache(SHIELD WATCH)

set(GEN_SHIELD_DERIVED_OVERLAY_SCRIPT    ${ZEPHYR_BASE}/scripts/build/gen_shield_derived_overlay.py)
set(GENERATED_SHIELDS_DIR                ${PROJECT_BINARY_DIR}/include/generated/shields)

# Add the directory derived overlay files to the SHIELD_DIRS output variable.
list(APPEND
  SHIELD_DIRS
  ${GENERATED_SHIELDS_DIR}
)

if(SHIELD)
  message(STATUS "Shield(s): ${SHIELD}")
endif()

if(DEFINED SHIELD)
  string(REPLACE " " ";" SHIELD_AS_LIST "${SHIELD}")
endif()
# SHIELD-NOTFOUND is a real CMake list, from which valid shields can be popped.
# After processing all shields, only invalid shields will be left in this list.
string(REGEX REPLACE "([@:][^;]*)" "" SHIELD-NOTFOUND "${SHIELD}")

foreach(root ${BOARD_ROOT})
  set(shield_dir ${root}/boards/shields)
  # Match the Kconfig.shield files in the shield directories to make sure we are
  # finding shields, e.g. x_nucleo_iks01a1/Kconfig.shield
  file(GLOB_RECURSE shields_refs_list ${shield_dir}/*/Kconfig.shield)

  # The above gives a list of Kconfig.shield files, like this:
  #
  # x_nucleo_iks01a1/Kconfig.shield;x_nucleo_iks01a2/Kconfig.shield
  #
  # we construct a list of shield names by extracting the directories
  # from each file and looking for <shield>.overlay files in there.
  # Each overlay corresponds to a shield. We obtain the shield name by
  # removing the .overlay extension.
  # We also create a SHIELD_DIR_${name} variable for each shield's directory.
  foreach(shields_refs ${shields_refs_list})
    get_filename_component(shield_path ${shields_refs} DIRECTORY)
    file(GLOB shield_overlays RELATIVE ${shield_path} ${shield_path}/*.overlay)
    foreach(overlay ${shield_overlays})
      get_filename_component(shield ${overlay} NAME_WE)
      list(APPEND SHIELD_LIST ${shield})
      set(SHIELD_DIR_${shield} ${shield_path})
    endforeach()
  endforeach()
endforeach()

# Process shields in-order
if(DEFINED SHIELD)
  foreach(shld ${SHIELD_AS_LIST})
    string(REGEX MATCH [[^([^@:]*)([@:].*)?$]] matched "${shld}")
    set(s ${CMAKE_MATCH_1})         # name part
    set(shield_opts ${CMAKE_MATCH_2}) # options part

    if(NOT ${s} IN_LIST SHIELD_LIST)
      continue()
    endif()

    list(REMOVE_ITEM SHIELD-NOTFOUND ${s})

    # Add the shield's directory to the SHIELD_DIRS output variable.
    list(APPEND
      SHIELD_DIRS
      ${SHIELD_DIR_${s}}
      )

    # get board-specific .conf and .overlay filenames
    zephyr_file(CONF_FILES ${SHIELD_DIR_${s}}/boards
                DTS   board_overlay_file
                KCONF board_conf_file
    )

    zephyr_file(CONF_FILES ${SHIELD_DIR_${s}}/boards/${s}
                DTS   board_shield_overlay_file
                KCONF board_shield_conf_file
    )

    get_filename_component(board_overlay_stem "${board_overlay_file}" NAME_WE)
    get_filename_component(shield_overlay_stem "${board_shield_overlay_file}" NAME_WE)

    set(shield_conf_list
        "${SHIELD_DIR_${s}}/${s}.conf"
        "${board_conf_file}"
        "${board_shield_conf_file}")

    set(shield_overlay_list
        "${SHIELD_DIR_${s}}/${s}.overlay"
        "${board_overlay_file}"
        "${board_shield_overlay_file}")

    set(derived_overlay_list
        "${GENERATED_SHIELDS_DIR}/${shld}.overlay"
        "${GENERATED_SHIELDS_DIR}/${shld}_${board_overlay_stem}.overlay"
        "${GENERATED_SHIELDS_DIR}/${shld}_${board_overlay_stem}_${shield_overlay_stem}.overlay")

    # Add board-specific .conf files to the shield_conf_files output variable.
    foreach(src_conf ${shield_conf_list})
      if(EXISTS ${src_conf})
        list(APPEND shield_conf_files ${src_conf})
      endif()
    endforeach()

    foreach(shield_overlay derived_overlay IN ZIP_LISTS shield_overlay_list derived_overlay_list)
      if (EXISTS ${shield_overlay})
        # Generate a derived overlay for each file, reflecting the options.
        execute_process(COMMAND
                        ${PYTHON_EXECUTABLE}
                        ${GEN_SHIELD_DERIVED_OVERLAY_SCRIPT}
                        "${shield_overlay}"
                        "--derived-overlay=${derived_overlay}"
                        "--shield-options=${shield_opts}"
                        COMMAND_ERROR_IS_FATAL ANY
                        )

        # Add the derived overlay file to the shield_dts_files output variable.
        list(APPEND shield_dts_files "${derived_overlay}")
      endif()
    endforeach()

  endforeach()
endif()

# Prepare shield usage command printing.
# This command prints all shields in the system in the following cases:
# - User specifies an invalid SHIELD
# - User invokes '<build-command> shields' target
list(SORT SHIELD_LIST)

if(DEFINED SHIELD AND NOT (SHIELD-NOTFOUND STREQUAL ""))
  # Convert the list to pure string with newlines for printing.
  string(REPLACE ";" "\n" shield_string "${SHIELD_LIST}")

  foreach (s ${SHIELD-NOTFOUND})
    message("No shield named '${s}' found")
  endforeach()
  message("Please choose from among the following shields:\n"
          "${shield_string}"
  )
  unset(CACHED_SHIELD CACHE)
  message(FATAL_ERROR "Invalid SHIELD; see above.")
endif()

# Prepend each shield with COMMAND <cmake> -E echo <shield>" for printing.
# Each shield is printed as new command because build files are not fond of newlines.
list(TRANSFORM SHIELD_LIST PREPEND "COMMAND;${CMAKE_COMMAND};-E;echo;"
     OUTPUT_VARIABLE shields_target_cmd
)

add_custom_target(shields ${shields_target_cmd} USES_TERMINAL)
