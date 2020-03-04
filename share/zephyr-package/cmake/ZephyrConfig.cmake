# SPDX-License-Identifier: Apache-2.0

# This file provides Zephyr Config Package functionality.
#
# The purpose of this files is to allow users to decide if they want to:
# - Use ZEPHYR_BASE environment setting for explicitly set select a zephyr installation
# - Support automatic Zephyr installation lookup through the use of find_package(ZEPHYR)

# First check to see if user has provided a Zephyr base manually.
# Set Zephyr base to environment setting.
# It will be empty if not set in environment.

include(${CMAKE_CURRENT_LIST_DIR}/zephyr_package_search.cmake)

macro(include_boilerplate location)
  set(Zephyr_FOUND True)
  if(NOT NO_BOILERPLATE)
    message("Including boilerplate (${location}): ${ZEPHYR_BASE}/cmake/app/boilerplate.cmake")
    include(${ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  endif()
endmacro()

set(ENV_ZEPHYR_BASE $ENV{ZEPHYR_BASE})
if((NOT DEFINED ZEPHYR_BASE) AND (DEFINED ENV_ZEPHYR_BASE))
  # Get rid of any double folder string before comparison, as example, user provides
  # ZEPHYR_BASE=//path/to//zephyr_base/
  # must also work.
  get_filename_component(ZEPHYR_BASE ${ENV_ZEPHYR_BASE} ABSOLUTE)
  set(ZEPHYR_BASE ${ZEPHYR_BASE} CACHE PATH "Zephyr base")
  include_boilerplate("Zephyr base")
  return()
endif()

if (DEFINED ZEPHYR_BASE)
  include_boilerplate("Zephyr base (cached)")
  return()
endif()

# If ZEPHYR_CANDIDATE is set, it means this file was include instead of called via find_package directly.
if(ZEPHYR_CANDIDATE)
  set(IS_INCLUDED TRUE)
endif()

# Find out the current Zephyr base.
get_filename_component(CURRENT_ZEPHYR_DIR ${CMAKE_CURRENT_LIST_FILE}/${ZEPHYR_RELATIVE_DIR} ABSOLUTE)
get_filename_component(CURRENT_WORKSPACE_DIR ${CMAKE_CURRENT_LIST_FILE}/${WORKSPACE_RELATIVE_DIR} ABSOLUTE)

string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "${CURRENT_ZEPHYR_DIR}/" COMMON_INDEX)
if (COMMON_INDEX EQUAL 0)
  # Project is in Zephyr repository.
  # We are in Zephyr repository.
  set(ZEPHYR_BASE ${CURRENT_ZEPHYR_DIR} CACHE PATH "Zephyr base")
  include_boilerplate("Zephyr repository")
  return()
endif()

if(IS_INCLUDED)
  # A higher level did the checking and included us and as we are not in Zephyr repository
  # (checked above) then we must be in Zephyr workspace.
  set(ZEPHYR_BASE ${CURRENT_ZEPHYR_DIR} CACHE PATH "Zephyr base")
  include_boilerplate("Zephyr workspace")
endif()

if(NOT IS_INCLUDED)
  string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "${CURRENT_WORKSPACE_DIR}/" COMMON_INDEX)
  if (COMMON_INDEX EQUAL 0)
    # Project is in Zephyr workspace.
    # This means this Zephyr is likely the correct one, but there could be an alternative installed along-side
    # Thus, check if there is an even better candidate.
    # This check works the following way.
    # CMake finds packages will look all packages registered in the user package registry.
    # As this code is processed inside registered packages, we simply test if another package has a
    # comon path with the current sample.
    # and if so, we will retrun here, and let CMake call into the other registered package for real
    # version checking.
    check_zephyr_package(CURRENT_WORKSPACE_DIR ${CURRENT_WORKSPACE_DIR})

    # We are the best candidate, so let's include boiler plate.
    set(ZEPHYR_BASE ${CURRENT_ZEPHYR_DIR} CACHE PATH "Zephyr base")
    include_boilerplate("Zephyr workspace")
    return()
  endif()

  check_zephyr_package(SEARCH_PARENTS)

  # Ending here means there were no candidates in workspace of the app.
  # Thus, the app is built as a Zephyr Freestanding application.
  # CMake find_package has already done the version checking, so let's just include boiler plate.
  # Previous find_package would have cleared Zephyr_FOUND variable, thus set it again.
  set(ZEPHYR_BASE ${CURRENT_ZEPHYR_DIR} CACHE PATH "Zephyr base")
  include_boilerplate("Freestanding")
endif()
