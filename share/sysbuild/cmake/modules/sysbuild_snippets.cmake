# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

zephyr_get(SB_SNIPPET)
if(NOT SB_SNIPPET AND SNIPPET)
  set_ifndef(SB_SNIPPET ${SNIPPET})
endif()

# Check for app-local snippets used with global SNIPPET variable.
# App-local snippets are only available to the specific image that defines them, so using
# them via global SNIPPET will cause other images to fail.
# We check this early at sysbuild level to provide a clear error message.
if(SNIPPET)
  set(app_snippets_dir "${APP_DIR}/snippets")
  if(EXISTS "${app_snippets_dir}")
    # Find all snippet.yml files in the app's snippets directory
    file(GLOB_RECURSE app_snippet_files "${app_snippets_dir}/*/snippet.yml"
                                        "${app_snippets_dir}/snippet.yml")

    # Extract snippet names from app-local snippet.yml files
    set(app_local_snippet_names)
    foreach(snippet_file IN LISTS app_snippet_files)
      file(STRINGS "${snippet_file}" snippet_lines)
      foreach(line IN LISTS snippet_lines)
        string(FIND "${line}" "name:" name_pos)
        if(name_pos EQUAL 0)
          string(SUBSTRING "${line}" 5 -1 snippet_name)
          string(STRIP "${snippet_name}" snippet_name)
          list(APPEND app_local_snippet_names "${snippet_name}")
          break()
        endif()
      endforeach()
    endforeach()

    # Check if any globally requested snippets are app-local
    string(REPLACE " " ";" snippet_list "${SNIPPET}")
    set(app_local_snippets_in_use)
    foreach(requested_snippet IN LISTS snippet_list)
      if("${requested_snippet}" IN_LIST app_local_snippet_names)
        list(APPEND app_local_snippets_in_use "${requested_snippet}")
      endif()
    endforeach()

    # If app-local snippets are used with global SNIPPET, emit fatal error
    if(app_local_snippets_in_use)
      cmake_path(GET APP_DIR FILENAME app_name)
      list(JOIN app_local_snippets_in_use ", " app_local_snippets_str)
      message(FATAL_ERROR
        "The following used snippet(s) are defined in the application's local "
        "snippets directory:\n"
        "    ${app_local_snippets_str}\n"
        "When using sysbuild, application-local snippets are only available to "
        "the main application. Using global SNIPPET variable will cause other images "
        "(e.g., bootloader) to fail.\n"
        "To fix this, use the image-specific snippet variable:\n"
        "    -D${app_name}_SNIPPET=${app_local_snippets_str}\n")
    endif()
  endif()
endif()

set(SNIPPET_NAMESPACE SB_SNIPPET)
set(SNIPPET_PYTHON_EXTRA_ARGS --sysbuild)
set(SNIPPET_APP_DIR ${APP_DIR})
include(${ZEPHYR_BASE}/cmake/modules/snippets.cmake)

set(SNIPPET_NAMESPACE)
set(SNIPPET_PYTHON_EXTRA_ARGS)
set(SNIPPET_APP_DIR)
