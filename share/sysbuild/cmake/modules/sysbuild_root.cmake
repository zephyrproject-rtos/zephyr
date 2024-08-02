# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024, Nordic Semiconductor ASA

# Convert Zephyr roots to absolute paths to be used by sysbuild.
#
# This CMake module will convert all relative paths in existing ROOT lists to
# absolute path relative from APP_DIR.
#
# Optional variables:
# - ARCH_ROOT:       CMake list of arch roots containing arch implementations
# - SOC_ROOT:        CMake list of SoC roots containing SoC implementations
# - BOARD_ROOT:      CMake list of board roots containing board and shield implementations
# - MODULE_EXT_ROOT: CMake list of module external roots containing module glue code
# - SCA_ROOT:        CMake list of SCA roots containing static code analysis integration code
#
# If a root is defined it will check the list of paths in the root and convert
# any relative path to absolute path and update the root list.
# If a root is undefined it will still be undefined when this module has loaded.
#
# Converted paths are placed in the CMake cache so that they are propagated
# correctly to image builds.

include_guard(GLOBAL)

include(extensions)

# Merge in variables from other sources
zephyr_get(MODULE_EXT_ROOT MERGE)
zephyr_get(BOARD_ROOT MERGE)
zephyr_get(SOC_ROOT MERGE)
zephyr_get(ARCH_ROOT MERGE)
zephyr_get(SCA_ROOT MERGE)

# Convert paths to absolute, relative from APP_DIR
zephyr_file(APPLICATION_ROOT MODULE_EXT_ROOT BASE_DIR ${APP_DIR})
zephyr_file(APPLICATION_ROOT BOARD_ROOT BASE_DIR ${APP_DIR})
zephyr_file(APPLICATION_ROOT SOC_ROOT BASE_DIR ${APP_DIR})
zephyr_file(APPLICATION_ROOT ARCH_ROOT BASE_DIR ${APP_DIR})
zephyr_file(APPLICATION_ROOT SCA_ROOT BASE_DIR ${APP_DIR})

# Sysbuild must ensure any locally defined variables in sysbuild/CMakeLists.txt
# have been added to the cache in order for the settings to propagate to images.
# note: zephyr_file has removed any list duplicates
if(DEFINED MODULE_EXT_ROOT)
  set(MODULE_EXT_ROOT ${MODULE_EXT_ROOT} CACHE PATH "Sysbuild adjusted MODULE_EXT_ROOT" FORCE)
endif()

if(DEFINED BOARD_ROOT)
  set(BOARD_ROOT ${BOARD_ROOT} CACHE PATH "Sysbuild adjusted BOARD_ROOT" FORCE)
endif()

if(DEFINED SOC_ROOT)
  set(SOC_ROOT ${SOC_ROOT} CACHE PATH "Sysbuild adjusted SOC_ROOT" FORCE)
endif()

if(DEFINED ARCH_ROOT)
  set(ARCH_ROOT ${ARCH_ROOT} CACHE PATH "Sysbuild adjusted ARCH_ROOT" FORCE)
endif()

if(DEFINED SCA_ROOT)
  set(SCA_ROOT ${SCA_ROOT} CACHE PATH "Sysbuild adjusted SCA_ROOT" FORCE)
endif()
