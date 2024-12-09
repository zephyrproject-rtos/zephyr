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

set(GENERATED_SHIELDS_DIR           ${PROJECT_BINARY_DIR}/include/generated/shields)
file(MAKE_DIRECTORY ${GENERATED_SHIELDS_DIR})

# Process shields in-order
if(DEFINED SHIELD)
  foreach(shld ${SHIELD_AS_LIST})
    string(REGEX MATCH
      [[^([a-zA-Z_][a-zA-Z0-9_]*)(@[a-zA-Z_][a-zA-Z0-9_]+)?(.*)?$]]
      matched
      "${shld}"
    )

    set(opts_list "")
    set(s ${CMAKE_MATCH_1}) # stem part

    if (NOT "${CMAKE_MATCH_3}" STREQUAL "")
      string(SUBSTRING ${CMAKE_MATCH_3} 1 -1 shld_opts) # options part
      string(REPLACE ":" ";" opts_list "${shld_opts}")
    endif()

    if (NOT "${CMAKE_MATCH_2}" STREQUAL "")
      string(SUBSTRING ${CMAKE_MATCH_2} 1 -1 shld_conn) # connector part
      list(APPEND opts_list "connector_name=${shld_conn}")
    endif()

    string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" shld_ident ${shld})
    string(TOUPPER ${shld_ident} derived_name)

    if(NOT ${s} IN_LIST SHIELD_LIST)
      continue()
    endif()

    list(REMOVE_ITEM SHIELD-NOTFOUND ${shld})

    set(derived_overlay ${GENERATED_SHIELDS_DIR}/${shld}.overlay)

    set(defines
      "#define SHIELD_DERIVED_NAME ${derived_name}\n"
      "#define SHIELD_DERIVED_NAME_DEFINED 1\n"
    )
    set(undefines
      "#undef SHIELD_DERIVED_NAME_DEFINED\n"
      "#undef SHIELD_DERIVED_NAME\n"
    )

    # generate option macros
    foreach (opt IN LISTS opts_list)
      if ("${opt}" STREQUAL "")
        continue()
      elseif (${opt} MATCHES [[^([a-zA-Z_][a-zA-Z0-9_]+)(=[a-zA-Z0-9_]*)?$]])
        if (NOT ${CMAKE_MATCH_2} STREQUAL "")
          string(SUBSTRING ${CMAKE_MATCH_2} 1 -1 opt_value)
        else()
          set(opt_value 1)
        endif()

        set(opt_name ${CMAKE_MATCH_1})
        string(TOUPPER "SHIELD_OPTION_${opt_name}" opt_fullname_upper)
        string(APPEND defines "#define ${opt_fullname_upper} ${opt_value}\n")
        string(APPEND defines "#define ${opt_fullname_upper}_DEFINED 1\n")
        string(PREPEND undefines "#undef ${opt_fullname_upper}\n")
        string(PREPEND undefines "#undef ${opt_fullname_upper}_DEFINED\n")
      endif()
    endforeach()

    # If a connector name is specified, a derived overlay will be created.
    # A derived overlay is a wrapper definition that defines SHIELD_DERIVED_NAME
    # and SHIELD_DERIVED_NAME_DEFINED and then loads the original overlay.

    file(APPEND ${derived_overlay}.new
      "/* auto-generated by shields.cmake, don't edit */\n"
      "\n"
      "#include <shield_utils.h>\n"
      ${defines}
      "\n"
      "#include <${SHIELD_DIR_${s}}/${s}.overlay>\n"
      "\n"
      ${undefines}
    )

    zephyr_file_copy(${derived_overlay}.new ${derived_overlay})
    file(REMOVE ${derived_overlay}.new)

    # Add generated <shield>@<conn_name>.overlay to the shield_dts_files output variable.
    list(APPEND
      shield_dts_files
      ${derived_overlay}
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
