# SPDX-License-Identifier: Apache-2.0

# This file provides Zephyr Config Package functionality.
#
# The purpose of this files is to allow users to decide if they want to:
# - Use ZEPHYR_BASE environment setting for explicitly set select a zephyr installation
# - Support automatic Zephyr installation lookup through the use of find_package(ZEPHYR)

# First check to see if user has provided a Zephyr base manually.
# Set Zephyr base to environment setting.
# It will be empty if not set in environment.

# Internal Zephyr CMake package message macro.
#
# This macro is only intended to be used within the Zephyr CMake package.
# The function `find_package()` supports an optional QUIET argument, and to
# honor that argument, the package_message() macro will not print messages when
# said flag has been given.
#
# Arguments to zephyr_package_message() are identical to regular CMake message()
# function.
macro(zephyr_package_message)
  if(NOT Zephyr_FIND_QUIETLY)
    message(${ARGN})
  endif()
endmacro()

macro(include_boilerplate location)
  set(Zephyr_DIR ${ZEPHYR_BASE}/share/zephyr-package/cmake CACHE PATH
      "The directory containing a CMake configuration file for Zephyr." FORCE
  )
  list(PREPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/cmake/modules)
  if(ZEPHYR_UNITTEST)
    zephyr_package_message(DEPRECATION "The ZephyrUnittest CMake package has been deprecated.\n"
                           "ZephyrUnittest has been replaced with Zephyr CMake module 'unittest' \n"
                           "and can be loaded as: 'find_package(Zephyr COMPONENTS unittest)'"
    )
    set(ZephyrUnittest_FOUND True)
    set(Zephyr_FIND_COMPONENTS unittest)
  else()
    set(Zephyr_FOUND True)
  endif()

  if(NOT DEFINED APPLICATION_SOURCE_DIR)
    set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH
        "Application Source Directory"
    )
  endif()

  if(NOT DEFINED APPLICATION_BINARY_DIR)
    set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH
        "Application Binary Directory"
    )
  endif()

  set(__build_dir ${APPLICATION_BINARY_DIR}/zephyr)
  set(PROJECT_BINARY_DIR ${__build_dir})

  if(NOT NO_BOILERPLATE)
    list(LENGTH Zephyr_FIND_COMPONENTS components_length)
    # The module messages are intentionally higher than STATUS to avoid the -- prefix
    # and make them more visible to users. This does result in them being output
    # to stderr, but that is an implementation detail of cmake.
    if(components_length EQUAL 0)
      zephyr_package_message(NOTICE "Loading Zephyr default modules (${location}).")
      include(zephyr_default NO_POLICY_SCOPE)
    else()
      string(JOIN " " msg_components ${Zephyr_FIND_COMPONENTS})
      zephyr_package_message(NOTICE "Loading Zephyr module(s) (${location}): ${msg_components}")
      foreach(component ${Zephyr_FIND_COMPONENTS})
        if(${component} MATCHES "^\([^:]*\):\(.*\)$")
          string(REPLACE "," ";" SUB_COMPONENTS ${CMAKE_MATCH_2})
          set(component ${CMAKE_MATCH_1})
        endif()
        include(${component})
      endforeach()
    endif()
  else()
    zephyr_package_message(DEPRECATION "The NO_BOILERPLATE setting has been deprecated.\n"
                           "Please use: 'find_package(Zephyr COMPONENTS <components>)'"
    )
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
else()
  include(${CMAKE_CURRENT_LIST_DIR}/zephyr_package_search.cmake)
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
    # common path with the current sample.
    # and if so, we will return here, and let CMake call into the other registered package for real
    # version checking.
    check_zephyr_package(CURRENT_WORKSPACE_DIR ${CURRENT_WORKSPACE_DIR})

    if(ZEPHYR_PREFER)
      check_zephyr_package(SEARCH_PARENTS CANDIDATES_PREFERENCE_LIST ${ZEPHYR_PREFER})
    endif()

    # We are the best candidate, so let's include boiler plate.
    set(ZEPHYR_BASE ${CURRENT_ZEPHYR_DIR} CACHE PATH "Zephyr base")
    include_boilerplate("Zephyr workspace")
    return()
  endif()

  check_zephyr_package(SEARCH_PARENTS CANDIDATES_PREFERENCE_LIST ${ZEPHYR_PREFER})

  # Ending here means there were no candidates in workspace of the app.
  # Thus, the app is built as a Zephyr Freestanding application.
  # CMake find_package has already done the version checking, so let's just include boiler plate.
  # Previous find_package would have cleared Zephyr_FOUND variable, thus set it again.
  set(ZEPHYR_BASE ${CURRENT_ZEPHYR_DIR} CACHE PATH "Zephyr base")
  include_boilerplate("Freestanding")
endif()
