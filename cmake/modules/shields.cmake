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

if(SHIELD)
  message(STATUS "Shield(s): ${SHIELD}")
endif()

if(DEFINED SHIELD)
  string(REPLACE " " ";" SHIELD_AS_LIST "${SHIELD}")
endif()
# SHIELD-NOTFOUND is a real CMake list, from which valid shields can be popped.
# After processing all shields, only invalid shields will be left in this list.
set(SHIELD-NOTFOUND ${SHIELD_AS_LIST})

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
  foreach(s ${SHIELD_AS_LIST})
    if(NOT ${s} IN_LIST SHIELD_LIST)
      continue()
    endif()

    list(REMOVE_ITEM SHIELD-NOTFOUND ${s})

    # Add <shield>.overlay to the shield_dts_files output variable.
    list(APPEND
      shield_dts_files
      ${SHIELD_DIR_${s}}/${s}.overlay
      )

    # Add the shield's directory to the SHIELD_DIRS output variable.
    list(APPEND
      SHIELD_DIRS
      ${SHIELD_DIR_${s}}
      )

    # Search for shield/shield.conf file
    if(EXISTS ${SHIELD_DIR_${s}}/${s}.conf)
      list(APPEND
        shield_conf_files
        ${SHIELD_DIR_${s}}/${s}.conf
        )
    endif()

    # Add board-specific .conf and .overlay files to their
    # respective output variables.
    zephyr_file(CONF_FILES ${SHIELD_DIR_${s}}/boards
                DTS   shield_dts_files
                KCONF shield_conf_files
    )
    zephyr_file(CONF_FILES ${SHIELD_DIR_${s}}/boards/${s}
                DTS   shield_dts_files
                KCONF shield_conf_files
    )
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
