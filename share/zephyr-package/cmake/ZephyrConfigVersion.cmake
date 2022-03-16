# SPDX-License-Identifier: Apache-2.0

# This file provides Zephyr Config Package version information.
#
# The purpose of the version file is to ensure that CMake find_package can correctly locate a
# usable Zephyr installation for building of applications.

# Checking for version 0.0.0 is a way to allow other Zephyr installation to determine if there is a better match.
# A better match would be an installed Zephyr that has a common index with current source dir.
# Version 0.0.0 indicates that we should just return, in order to obtain our path.
if(0.0.0 STREQUAL PACKAGE_FIND_VERSION)
  return()
endif()

macro(check_zephyr_version)
  if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
    if(IS_INCLUDED)
      # We are just a candidate, meaning we have been included from other installed module.
      message("\n  The following Zephyr repository configuration file were considered but not accepted:")
      message("\n    ${CMAKE_CURRENT_LIST_FILE}, version: ${PACKAGE_VERSION}\n")
    endif()

    set(PACKAGE_VERSION_COMPATIBLE FALSE)
  else()
    # For now, Zephyr is capable to find the right base on all older versions as long as they define
    # a Zephyr config package (This code)
    # In future, this is the place to update in case Zephyr 3.x is not backward compatible with version 2.x
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
    if(PACKAGE_FIND_VERSION STREQUAL PACKAGE_VERSION)
      set(PACKAGE_VERSION_EXACT TRUE)
    endif()
  endif()
endmacro()

# First check to see if user has provided a Zephyr base manually and it is first run (cache not set).
set(ENV_ZEPHYR_BASE $ENV{ZEPHYR_BASE})
if((NOT DEFINED ZEPHYR_BASE) AND (DEFINED ENV_ZEPHYR_BASE))
  # Get rid of any double folder string before comparison, as example, user provides
  # ZEPHYR_BASE=//path/to//zephyr_base/
  # must also work.
  get_filename_component(ZEPHYR_BASE $ENV{ZEPHYR_BASE} ABSOLUTE)
endif()

# If ZEPHYR_CANDIDATE is set, it means this file was include instead of called via find_package directly.
if(ZEPHYR_CANDIDATE)
  set(IS_INCLUDED TRUE)
else()
  include(${CMAKE_CURRENT_LIST_DIR}/zephyr_package_search.cmake)
endif()

if((DEFINED ZEPHYR_BASE) OR (DEFINED ENV_ZEPHYR_BASE))
  # ZEPHYR_BASE was set in cache from earlier run or in environment (first run),
  # meaning the package version must be ignored and the Zephyr pointed to by
  # ZEPHYR_BASE is to be used regardless of version.
  if (${ZEPHYR_BASE}/share/zephyr-package/cmake STREQUAL ${CMAKE_CURRENT_LIST_DIR})
    # We are the Zephyr to be used

    set(NO_PRINT_VERSION True)
    include(${ZEPHYR_BASE}/cmake/modules/version.cmake)
    # Zephyr uses project version, but CMake package uses PACKAGE_VERSION
    set(PACKAGE_VERSION ${PROJECT_VERSION})
    check_zephyr_version()

    if(IS_INCLUDED)
      # We are included, so we need to ensure that the version of the top-level
      # package file is returned. This Zephyr version has already been printed
      # as part of `check_zephyr_version()`
      if(NOT ${PACKAGE_VERSION_COMPATIBLE}
        OR (Zephyr_FIND_VERSION_EXACT AND NOT PACKAGE_VERSION_EXACT)
      )
        # When Zephyr base is set and we are checked as an included file
        # (IS_INCLUDED=True), then we are unable to retrieve the version of the
        # parent Zephyr, therefore just mark it as ignored.
        set(PACKAGE_VERSION "ignored (ZEPHYR_BASE is set)")
      endif()
    endif()
  elseif ((NOT IS_INCLUDED) AND (DEFINED ZEPHYR_BASE))
    check_zephyr_package(ZEPHYR_BASE ${ZEPHYR_BASE} VERSION_CHECK)
  else()
    # User has pointed to a different Zephyr installation, so don't use this version
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
  endif()
  return()
endif()

# Find out the current Zephyr base.
get_filename_component(CURRENT_ZEPHYR_DIR ${CMAKE_CURRENT_LIST_DIR}/../../.. ABSOLUTE)
get_filename_component(CURRENT_WORKSPACE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../.. ABSOLUTE)

# Temporary set local Zephyr base to allow using version.cmake to find this Zephyr repository current version
set(ZEPHYR_BASE ${CURRENT_ZEPHYR_DIR})

# Tell version.cmake to not print as printing version for all Zephyr installations being tested
# will lead to confusion on which is being used.
set(NO_PRINT_VERSION True)
include(${ZEPHYR_BASE}/cmake/modules/version.cmake)
# Zephyr uses project version, but CMake package uses PACKAGE_VERSION
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(ZEPHYR_BASE)

# Do we share common index, if so, this is the correct version to check.
string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "${CURRENT_ZEPHYR_DIR}/" COMMON_INDEX)
if (COMMON_INDEX EQUAL 0)
  # Project is a Zephyr repository app.

  check_zephyr_version()
  return()
endif()

if(NOT IS_INCLUDED)
  # Only do this if we are an installed CMake Config package and checking for workspace candidates.

  string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "${CURRENT_WORKSPACE_DIR}/" COMMON_INDEX)
  if (COMMON_INDEX EQUAL 0)
    # Project is a Zephyr workspace app.
    # This means this Zephyr is likely the correct one, but there could be an alternative installed along-side
    # Thus, check if there is an even better candidate.
    check_zephyr_package(CURRENT_WORKSPACE_DIR ${CURRENT_WORKSPACE_DIR} VERSION_CHECK)

    # We are the best candidate, so let's check our own version.
    check_zephyr_version()
    return()
  endif()

  # Checking for installed candidates which could also be an workspace candidates.
  # This check works the following way.
  # CMake finds packages will look all packages registered in the user package registry.
  # As this code is processed inside registered packages, we simply test if
  # another package has a common path with the current sample, and if so, we
  # will return here, and let CMake call into the other registered package for
  # real version checking.
  check_zephyr_package(CHECK_ONLY VERSION_CHECK)

  # Check for workspace candidates.
  check_zephyr_package(SEARCH_PARENTS VERSION_CHECK)
endif()

# Ending here means there were no candidates in workspace of the app.
# Thus, the app is built as a Zephyr Freestanding application.
# Let's do basic CMake version checking.
check_zephyr_version()
