# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

set(EXTRA_KCONFIG_TARGET_COMMAND_FOR_sysbuild_menuconfig
  ${ZEPHYR_BASE}/scripts/kconfig/menuconfig.py
)

set(EXTRA_KCONFIG_TARGET_COMMAND_FOR_sysbuild_guiconfig
  ${ZEPHYR_BASE}/scripts/kconfig/guiconfig.py
)

set(KCONFIG_TARGETS sysbuild_menuconfig sysbuild_guiconfig)
list(TRANSFORM EXTRA_KCONFIG_TARGETS PREPEND "sysbuild_")

zephyr_get(APPLICATION_CONFIG_DIR)
if(DEFINED APPLICATION_CONFIG_DIR)
  zephyr_file(APPLICATION_ROOT APPLICATION_CONFIG_DIR BASE_DIR ${APP_DIR})

  # Sysbuild must add a locally defined APPLICATION_CONFIG_DIR in sysbuild/CMakeLists.txt
  # to the cache in order for the setting to propagate to images.
  set(APPLICATION_CONFIG_DIR ${APPLICATION_CONFIG_DIR} CACHE PATH
      "Sysbuild adjusted APPLICATION_CONFIG_DIR" FORCE
  )
endif()

zephyr_get(SB_APPLICATION_CONFIG_DIR)
if(DEFINED SB_APPLICATION_CONFIG_DIR)
  zephyr_file(APPLICATION_ROOT SB_APPLICATION_CONFIG_DIR BASE_DIR ${APP_DIR})
  set(APPLICATION_CONFIG_DIR ${SB_APPLICATION_CONFIG_DIR})
endif()
set_ifndef(APPLICATION_CONFIG_DIR ${APP_DIR})
string(CONFIGURE ${APPLICATION_CONFIG_DIR} APPLICATION_CONFIG_DIR)

if(DEFINED SB_CONF_FILE)
  # SB_CONF_FILE already set so nothing to do.
elseif(DEFINED ENV{SB_CONF_FILE})
  set(SB_CONF_FILE $ENV{SB_CONF_FILE})
else()
  # sysbuild.conf is an optional file, because sysbuild is an opt-in feature.
  zephyr_file(CONF_FILES ${APPLICATION_CONFIG_DIR} KCONF SB_CONF_FILE NAMES "sysbuild.conf" SUFFIX ${FILE_SUFFIX})
endif()

if(NOT DEFINED SB_EXTRA_CONF_FILE AND DEFINED SB_OVERLAY_CONFIG)
  set(SB_EXTRA_CONF_FILE ${SB_OVERLAY_CONFIG})
endif()

# Let SB_CONF_FILE and SB_EXTRA_CONF_FILE be relative to APP_DIR.
# Either variable can be a list of paths, so we must make all of them absolute.
foreach(conf_file_var SB_CONF_FILE SB_EXTRA_CONF_FILE)
  if(DEFINED ${conf_file_var})
    string(CONFIGURE "${${conf_file_var}}" conf_file_expanded)
    set(${conf_file_var} "")
    foreach(conf_file ${conf_file_expanded})
      cmake_path(ABSOLUTE_PATH conf_file BASE_DIRECTORY ${APP_DIR})
      list(APPEND ${conf_file_var} ${conf_file})
    endforeach()
  endif()
endforeach()

if(DEFINED SB_CONF_FILE AND NOT DEFINED CACHE{SB_CONF_FILE})
  # We only want to set this in cache it has been defined and is not already there.
  set(SB_CONF_FILE ${SB_CONF_FILE} CACHE STRING "If desired, you can build the application with \
  SYSbuild configuration settings specified in an alternate .conf file using this parameter. \
  These settings will override the settings in the application’s SYSBuild config file or its \
  default .conf file. Multiple files may be listed, e.g. SB_CONF_FILE=\"sys1.conf sys2.conf\"")
endif()

if(NOT DEFINED SB_CONF_FILE)
  # If there is no SB_CONF_FILE, then use empty.conf to make kconfiglib happy.
  # Not adding it to CMake cache ensures that a later created sysbuild.conf
  # will be automatically detected.
  set(SB_CONF_FILE ${CMAKE_CURRENT_BINARY_DIR}/empty.conf)
endif()

if(NOT EXISTS "${APPLICATION_CONFIG_DIR}")
  message(FATAL_ERROR "${APPLICATION_CONFIG_DIR} (APPLICATION_CONFIG_DIR) doesn't exist. "
          "Unable to lookup sysbuild.conf or other related sysbuild configuration files. "
	  "Please ensure APPLICATION_CONFIG_DIR points to a valid location."
  )
endif()

# Empty files to make kconfig.py happy.
file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/empty.conf)
set(APPLICATION_SOURCE_DIR ${sysbuild_toplevel_SOURCE_DIR})
set(AUTOCONF_H             ${CMAKE_CURRENT_BINARY_DIR}/autoconf.h)
set(CONF_FILE              ${SB_CONF_FILE})
set(EXTRA_CONF_FILE        "${SB_EXTRA_CONF_FILE}")
set(BOARD_DEFCONFIG        "${CMAKE_CURRENT_BINARY_DIR}/empty.conf")
if(DEFINED BOARD_REVISION)
  set(BOARD_REVISION_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/empty.conf")
endif()

# Unset shield configuration files if set to prevent including in sysbuild
set(shield_conf_files)

list(APPEND ZEPHYR_KCONFIG_MODULES_DIR BOARD=${BOARD})
set(KCONFIG_NAMESPACE SB_CONFIG)

if(EXISTS ${APP_DIR}/Kconfig.sysbuild)
  set(KCONFIG_ROOT ${APP_DIR}/Kconfig.sysbuild)
endif()
include(${ZEPHYR_BASE}/cmake/modules/kconfig.cmake)
set(CONF_FILE)
set(EXTRA_CONF_FILE)
