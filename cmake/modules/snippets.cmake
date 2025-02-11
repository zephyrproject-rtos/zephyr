# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Snippets support
#
# This module:
#
# - searches for snippets in zephyr and any modules
# - validates the SNIPPET input variable, if any
#
# If SNIPPET contains a snippet name that is not found, an error
# will be raised, and the list of valid snippets will be printed.
#
# Outcome:
# The following variables will be defined when this module completes:
#
# - SNIPPET_AS_LIST: CMake list of snippet names, created from the
#   SNIPPET variable
# - SNIPPET_ROOT: CMake list of snippet roots, deduplicated and with
#   ZEPHYR_BASE appended at the end
#
# The following variables may be updated when this module completes:
# - DTC_OVERLAY_FILE
# - OVERLAY_CONFIG
#
# The following targets will be defined when this CMake module completes:
# - snippets: when invoked, a list of valid snippets will be printed
#
# Optional variables:
# - SNIPPET_ROOT: input CMake list of snippet roots (directories containing
#   additional snippet implementations); this should not include ZEPHYR_BASE,
#   as that will be added by this module

include_guard(GLOBAL)

include(extensions)

# Warn the user if SNIPPET changes later. Such changes are ignored.
zephyr_check_cache(SNIPPET WATCH)

# Putting the body into a function prevents us from polluting the
# parent scope. We'll set our outcome variables in the parent scope of
# the function to ensure the outcome of the module.
function(zephyr_process_snippets)
  set(snippets_py ${ZEPHYR_BASE}/scripts/snippets.py)
  set(snippets_generated ${CMAKE_BINARY_DIR}/zephyr/snippets_generated.cmake)

  # Set SNIPPET_AS_LIST, removing snippets_generated.cmake if we are
  # running cmake again and snippets are no longer requested.
  if (NOT DEFINED SNIPPET)
    set(SNIPPET_AS_LIST "" PARENT_SCOPE)
    file(REMOVE ${snippets_generated})
  else()
    string(REPLACE " " ";" SNIPPET_AS_LIST "${SNIPPET}")
    set(SNIPPET_AS_LIST "${SNIPPET_AS_LIST}" PARENT_SCOPE)
  endif()

  # Set SNIPPET_ROOT.
  list(APPEND SNIPPET_ROOT ${APPLICATION_SOURCE_DIR})
  list(APPEND SNIPPET_ROOT ${ZEPHYR_BASE})
  unset(real_snippet_root)
  foreach(snippet_dir ${SNIPPET_ROOT})
    # The user might have put a symbolic link in here, for example.
    file(REAL_PATH ${snippet_dir} real_snippet_dir)
    list(APPEND real_snippet_root ${real_snippet_dir})
  endforeach()
  set(SNIPPET_ROOT ${real_snippet_root})
  list(REMOVE_DUPLICATES SNIPPET_ROOT)
  set(SNIPPET_ROOT "${SNIPPET_ROOT}" PARENT_SCOPE)

  # Generate and include snippets_generated.cmake.
  # The Python script is responsible for checking for unknown
  # snippets.
  set(snippet_root_args)
  foreach(root IN LISTS SNIPPET_ROOT)
    list(APPEND snippet_root_args --snippet-root "${root}")
  endforeach()
  set(requested_snippet_args)
  foreach(snippet_name ${SNIPPET_AS_LIST})
    list(APPEND requested_snippet_args --snippet "${snippet_name}")
  endforeach()
  execute_process(COMMAND ${PYTHON_EXECUTABLE}
    ${snippets_py}
    ${snippet_root_args}
    ${requested_snippet_args}
    --cmake-out ${snippets_generated}
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
    RESULT_VARIABLE ret)
  if(${ret})
    message(FATAL_ERROR "${output}")
  endif()
  include(${snippets_generated})

  # Create the 'snippets' target. Each snippet is printed in a
  # separate command because build system files are not fond of
  # newlines.
  list(TRANSFORM SNIPPET_NAMES PREPEND "COMMAND;${CMAKE_COMMAND};-E;echo;"
    OUTPUT_VARIABLE snippets_target_cmd)
  add_custom_target(snippets ${snippets_target_cmd} USES_TERMINAL)

  # If snippets were requested, print messages for each one.
  if(SNIPPET_AS_LIST)
    # Print the requested snippets.
    set(snippet_names "Snippet(s):")
    foreach(snippet IN LISTS SNIPPET_AS_LIST)
      string(APPEND snippet_names " ${snippet}")
    endforeach()
    message(STATUS "${snippet_names}")
  endif()

  # Re-run cmake if any files we depend on changed.
  set_property(DIRECTORY APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS
    ${snippets_py}
    ${SNIPPET_PATHS}            #  generated variable
    )
endfunction()

zephyr_process_snippets()
