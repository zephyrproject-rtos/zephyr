# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024, Nordic Semiconductor ASA

#[=======================================================================[.rst:
sysbuild_root
#############

Convert Zephyr roots to absolute paths to be used by sysbuild.

This module converts all relative paths in the following root lists to absolute paths, relative
from :cmake:variable:`APP_DIR`:

* :cmake:variable:`ARCH_ROOT`
* :cmake:variable:`BOARD_ROOT`
* :cmake:variable:`MODULE_EXT_ROOT`
* :cmake:variable:`SCA_ROOT`
* :cmake:variable:`SNIPPET_ROOT`
* :cmake:variable:`SOC_ROOT`

If a root is defined, this module checks the list of paths in the root, converts any relative path
to an absolute path, and updates the root list. If a root is undefined, it is still undefined once
this module has loaded.

Converted paths are placed in the CMake cache so that they are propagated correctly to image
builds.

#]=======================================================================]

include_guard(GLOBAL)

include(extensions)

# Merge in variables from other sources
zephyr_get(MODULE_EXT_ROOT MERGE)
zephyr_get(BOARD_ROOT MERGE)
zephyr_get(SOC_ROOT MERGE)
zephyr_get(ARCH_ROOT MERGE)
zephyr_get(SCA_ROOT MERGE)
zephyr_get(SNIPPET_ROOT MERGE)

# Convert paths to absolute, relative from APP_DIR
zephyr_file(APPLICATION_ROOT MODULE_EXT_ROOT BASE_DIR ${APP_DIR})
zephyr_file(APPLICATION_ROOT BOARD_ROOT BASE_DIR ${APP_DIR})
zephyr_file(APPLICATION_ROOT SOC_ROOT BASE_DIR ${APP_DIR})
zephyr_file(APPLICATION_ROOT ARCH_ROOT BASE_DIR ${APP_DIR})
zephyr_file(APPLICATION_ROOT SCA_ROOT BASE_DIR ${APP_DIR})
zephyr_file(APPLICATION_ROOT SNIPPET_ROOT BASE_DIR ${APP_DIR})

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

if(DEFINED SNIPPET_ROOT)
  set(SNIPPET_ROOT ${SNIPPET_ROOT} CACHE PATH "Sysbuild adjusted SNIPPET_ROOT" FORCE)
endif()
