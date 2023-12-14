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

if(DEFINED SB_CONF_FILE)
  # SB_CONF_FILE already set so nothing to do.
elseif(DEFINED ENV{SB_CONF_FILE})
  set(SB_CONF_FILE $ENV{SB_CONF_FILE})
elseif(EXISTS      ${APP_DIR}/sysbuild.conf)
  set(SB_CONF_FILE ${APP_DIR}/sysbuild.conf)
else()
  # Because SYSBuild is opt-in feature, then it is permitted to not have a
  # SYSBuild dedicated configuration file.
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

# Empty files to make kconfig.py happy.
file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/empty.conf)
set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
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
