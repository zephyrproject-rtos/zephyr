# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026, Analog Devices Inc.

# Auto-discover DTS overlays and Kconfig fragments from OVERLAY_ROOT directories.
#
# For each directory listed in OVERLAY_ROOT, all *.overlay files are appended
# to EXTRA_DTC_OVERLAY_FILE and all *.conf files are appended to EXTRA_CONF_FILE.
# Only files directly in the directory are matched; subdirectories (e.g. boards/)
# are not recursed into since board-specific overlays in APPLICATION_CONFIG_DIR
# are already handled by configuration_files.cmake.
#
# OVERLAY_ROOT directories are processed in list order, so later entries take
# precedence over earlier ones when there are DTS or Kconfig conflicts.

include_guard(GLOBAL)

include(extensions)

zephyr_get(OVERLAY_ROOT MERGE SYSBUILD GLOBAL)
if(NOT OVERLAY_ROOT)
  return()
endif()

foreach(root IN LISTS OVERLAY_ROOT)
  if(NOT IS_DIRECTORY "${root}")
    message(WARNING "OVERLAY_ROOT entry is not a directory: ${root}")
    continue()
  endif()

  file(GLOB _overlays CONFIGURE_DEPENDS "${root}/*.overlay")
  foreach(f IN LISTS _overlays)
    message(STATUS "OVERLAY_ROOT: adding DTS overlay ${f}")
    zephyr_set(EXTRA_DTC_OVERLAY_FILE ${f} SCOPE snippets APPEND)
  endforeach()

  file(GLOB _confs CONFIGURE_DEPENDS "${root}/*.conf")
  foreach(f IN LISTS _confs)
    message(STATUS "OVERLAY_ROOT: adding Kconfig fragment ${f}")
    zephyr_set(EXTRA_CONF_FILE ${f} SCOPE snippets APPEND)
  endforeach()
endforeach()
