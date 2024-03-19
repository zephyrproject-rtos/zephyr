# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Zephyr build system configuration files.
#
# Locate the Kconfig and DT config files that are to be used.
# Also, locate the appropriate application config directory.
#
# Outcome:
# The following variables will be defined when this CMake module completes:
#
# - CONF_FILE:              List of Kconfig fragments
# - EXTRA_CONF_FILE:        List of additional Kconfig fragments
# - DTC_OVERLAY_FILE:       List of devicetree overlay files
# - EXTRA_DTC_OVERLAY_FILE  List of additional devicetree overlay files
# - DTS_EXTRA_CPPFLAGS      List of additional devicetree preprocessor defines
# - APPLICATION_CONFIG_DIR: Root folder for application configuration
#
# If any of the above variables are already set when this CMake module is
# loaded, then no changes to the variable will happen.
#
# Variables set by this module and not mentioned above are considered internal
# use only and may be removed, renamed, or re-purposed without prior notice.

include_guard(GLOBAL)

include(extensions)

# Merge in variables from other sources (e.g. sysbuild)
zephyr_get(FILE_SUFFIX SYSBUILD GLOBAL)

zephyr_get(APPLICATION_CONFIG_DIR)
if(DEFINED APPLICATION_CONFIG_DIR)
  string(CONFIGURE ${APPLICATION_CONFIG_DIR} APPLICATION_CONFIG_DIR)
  if(NOT IS_ABSOLUTE ${APPLICATION_CONFIG_DIR})
    get_filename_component(APPLICATION_CONFIG_DIR ${APPLICATION_CONFIG_DIR} ABSOLUTE)
  endif()
else()
  # Application config dir is not set, so we default to the  application
  # source directory as configuration directory.
  set(APPLICATION_CONFIG_DIR ${APPLICATION_SOURCE_DIR})
endif()

zephyr_get(CONF_FILE SYSBUILD LOCAL)
if(NOT DEFINED CONF_FILE)
  zephyr_file(CONF_FILES ${APPLICATION_CONFIG_DIR} KCONF CONF_FILE NAMES "prj.conf" SUFFIX ${FILE_SUFFIX} REQUIRED)
  zephyr_file(CONF_FILES ${APPLICATION_CONFIG_DIR}/boards KCONF CONF_FILE)
else()
  string(CONFIGURE "${CONF_FILE}" CONF_FILE_EXPANDED)
  string(REPLACE " " ";" CONF_FILE_AS_LIST "${CONF_FILE_EXPANDED}")
  list(LENGTH CONF_FILE_AS_LIST CONF_FILE_LENGTH)
  if(${CONF_FILE_LENGTH} EQUAL 1)
    get_filename_component(CONF_FILE_NAME ${CONF_FILE} NAME)
    if(${CONF_FILE_NAME} MATCHES "prj_(.*).conf")
      set(CONF_FILE_BUILD_TYPE ${CMAKE_MATCH_1})
      zephyr_file(CONF_FILES ${APPLICATION_CONFIG_DIR}/boards KCONF CONF_FILE
                  BUILD ${CONF_FILE_BUILD_TYPE}
      )
      set(CONF_FILE_FORCE_CACHE FORCE)
    endif()
  endif()
endif()

set(APPLICATION_CONFIG_DIR ${APPLICATION_CONFIG_DIR} CACHE INTERNAL "The application configuration folder" FORCE)
set(CONF_FILE ${CONF_FILE} CACHE STRING "If desired, you can build the application using\
the configuration settings specified in an alternate .conf file using this parameter. \
These settings will override the settings in the application’s .config file or its default .conf file.\
Multiple files may be listed, e.g. CONF_FILE=\"prj1.conf;prj2.conf\" \
The CACHED_CONF_FILE is internal Zephyr variable used between CMake runs. \
To change CONF_FILE, use the CONF_FILE variable." ${CONF_FILE_FORCE_CACHE})

# The CONF_FILE variable is now set to its final value.
zephyr_boilerplate_watch(CONF_FILE)

zephyr_file(CONF_FILES ${APPLICATION_CONFIG_DIR}/boards DTS APP_BOARD_DTS SUFFIX ${FILE_SUFFIX})

zephyr_get(DTC_OVERLAY_FILE SYSBUILD LOCAL)
if(NOT DEFINED DTC_OVERLAY_FILE)
  zephyr_build_string(board_overlay_strings
                      BOARD ${BOARD}
                      BOARD_QUALIFIERS ${BOARD_QUALIFIERS}
                      MERGE
  )
  list(TRANSFORM board_overlay_strings APPEND ".overlay")

  zephyr_file(CONF_FILES ${APPLICATION_CONFIG_DIR} DTS DTC_OVERLAY_FILE
              NAMES "${APP_BOARD_DTS};${board_overlay_strings};app.overlay" SUFFIX ${FILE_SUFFIX})
endif()

set(DTC_OVERLAY_FILE ${DTC_OVERLAY_FILE} CACHE STRING "If desired, you can \
build the application using the DT configuration settings specified in an \
alternate .overlay file using this parameter. These settings will override the \
settings in the board's .dts file. Multiple files may be listed, e.g. \
DTC_OVERLAY_FILE=\"dts1.overlay dts2.overlay\"")

# The DTC_OVERLAY_FILE variable is now set to its final value.
zephyr_boilerplate_watch(DTC_OVERLAY_FILE)

zephyr_get(EXTRA_CONF_FILE SYSBUILD LOCAL VAR EXTRA_CONF_FILE OVERLAY_CONFIG MERGE REVERSE)
zephyr_get(EXTRA_DTC_OVERLAY_FILE SYSBUILD LOCAL MERGE REVERSE)
zephyr_get(DTS_EXTRA_CPPFLAGS SYSBUILD LOCAL MERGE REVERSE)
