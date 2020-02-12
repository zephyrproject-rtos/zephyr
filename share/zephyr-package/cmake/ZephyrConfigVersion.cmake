# SPDX-License-Identifier: Apache-2.0

# This file provides Zephyr Config Package version information.
#
# The purpose of the version file is to ensure that CMake find_package can correctly locate a
# usable Zephyr installation for building of applications.

# First check to see if user has provided a Zephyr base manually.
set(ZEPHYR_BASE $ENV{ZEPHYR_BASE})

if (ZEPHYR_BASE)
  # ZEPHYR_BASE was set in environment, meaning the package version must be ignored and the Zephyr
  # pointed to by ZEPHYR_BASE is to be used regardless of version

  # Get rid of any double folder string before comparison, as example, user provides
  # ZEPHYR_BASE=//path/to//zephyr_base/
  # must also work.
  get_filename_component(ZEPHYR_BASE ${ZEPHYR_BASE} ABSOLUTE)
  if (${ZEPHYR_BASE}/zephyr-package/cmake STREQUAL ${CMAKE_CURRENT_LIST_DIR})
    # We are the Zephyr to be used
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
    set(PACKAGE_VERSION_EXACT TRUE)
  else()
    # User has pointed to a different Zephyr installation, so don't use this version
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
  endif()
  return()
endif()

# Temporary set local Zephyr base to allow using version.cmake to find this Zephyr tree current version
set(ZEPHYR_BASE ${CMAKE_CURRENT_LIST_DIR}/../../..)
include(${ZEPHYR_BASE}/cmake/version.cmake)
set(ZEPHYR_BASE)

# Zephyr uses project version, but CMake package uses PACKAGE_VERSION
set(PACKAGE_VERSION ${PROJECT_VERSION})
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  # Currently, this version is capable of handling on prior versions.
  # In future, in case version 3.0.0 cannot be used for project requiring
  # version 2.x.x, then add such check here.
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  if(PACKAGE_FIND_VERSION STREQUAL PACKAGE_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
