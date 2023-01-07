# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Snippets support
#
# Outcome:
# The following variables will be defined when this module completes:
#
# - SNIPPET_AS_LIST: CMake list of snippet names, created from the
#   SNIPPET variable

include_guard(GLOBAL)

include(extensions)

# Warn the user if SNIPPET changes later. Such changes are ignored.
zephyr_check_cache(SNIPPET WATCH)

# Putting the body into a function prevents us from polluting the
# parent scope. We'll set our outcome variables in the parent scope of
# the function to ensure the outcome of the module.
function(zephyr_process_snippets)
  if (SNIPPET)
    message(STATUS "Snippet(s): ${SNIPPET}")
  else()
    set(SNIPPET_AS_LIST "" PARENT_SCOPE)
    return()
  endif()

  string(REPLACE " " ";" SNIPPET_AS_LIST "${SNIPPET}")
  set(SNIPPET_AS_LIST "${SNIPPET_AS_LIST}" PARENT_SCOPE)
endfunction()

zephyr_process_snippets()
