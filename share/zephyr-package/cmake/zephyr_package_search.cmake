# The purpose of this file is to provide search mechanism for locating Zephyr in-work-tree package
# even when they are not installed into CMake package system
# Linux/MacOS: ~/.cmake/packages
# Windows:     Registry database

# Relative directory of workspace project dir as seen from Zephyr package file
set(WORKSPACE_RELATIVE_DIR "../../../../..")

# Relative directory of Zephyr dir as seen from Zephyr package file
set(ZEPHYR_RELATIVE_DIR "../../../..")

# This macro returns a list of parent folders to use for later searches.
macro(get_search_paths START_PATH SEARCH_PATHS)
  get_filename_component(SEARCH_PATH ${START_PATH} DIRECTORY)
  while(NOT (SEARCH_PATH STREQUAL "/"))
    list(APPEND SEARCH_PATHS ${SEARCH_PATH}/zephyr)
    list(APPEND SEARCH_PATHS ${SEARCH_PATH})
    get_filename_component(SEARCH_PATH ${SEARCH_PATH} DIRECTORY)
  endwhile()
endmacro()

# This macro can check for additional Zephyr package that has a better match
# Options:
# - ZEPHYR_BASE         : Use the specified ZEPHYR_BASE directly.
# - WORKSPACE_DIR       : Search for projects in specified  workspace.
# - SEARCH_PARENTS      : Search parent folder of current source file (application) to locate in-project-tree Zephyr candidates.
# - CHECK_ONLY          : Only set PACKAGE_VERSION_COMPATIBLE to false if a better candidate is found, default is to also include the found candidate.
# - VERSION_CHECK       : This is the version check stage by CMake find package
macro(check_zephyr_package)
  set(options CHECK_ONLY INCLUDE_FILE SEARCH_PARENTS VERSION_CHECK)
  set(single_args WORKSPACE_DIR ZEPHYR_BASE)
  cmake_parse_arguments(CHECK_ZEPHYR_PACKAGE "${options}" "${single_args}" "" ${ARGN})

  if(CHECK_ZEPHYR_PACKAGE_ZEPHYR_BASE)
    set(SEARCH_SETTINGS PATHS ${CHECK_ZEPHYR_PACKAGE_ZEPHYR_BASE} NO_DEFAULT_PATH)
  endif()

  if(CHECK_ZEPHYR_PACKAGE_WORKSPACE_DIR)
    set(SEARCH_SETTINGS PATHS ${CHECK_ZEPHYR_PACKAGE_WORKSPACE_DIR}/zephyr ${CHECK_ZEPHYR_PACKAGE_WORKSPACE_DIR} NO_DEFAULT_PATH)
  endif()

  if(CHECK_ZEPHYR_PACKAGE_SEARCH_PARENTS)
    get_search_paths(${CMAKE_CURRENT_SOURCE_DIR} SEARCH_PATHS)
    set(SEARCH_SETTINGS PATHS ${SEARCH_PATHS} NO_DEFAULT_PATH)
  endif()

  # Searching for version zero means there will be no match, but we obtain
  # a list of all potential Zephyr candidates in the tree to consider.
  find_package(Zephyr 0.0.0 EXACT QUIET ${SEARCH_SETTINGS})

  # The find package will also find ourself when searching in-tree, so to avoid re-including
  # this file, it is removed from the list along with any duplicates.
  list(REMOVE_ITEM Zephyr_CONSIDERED_CONFIGS ${CMAKE_CURRENT_LIST_DIR}/ZephyrConfig.cmake)
  list(REMOVE_DUPLICATES Zephyr_CONSIDERED_CONFIGS)

  foreach(ZEPHYR_CANDIDATE ${Zephyr_CONSIDERED_CONFIGS})
    if(CHECK_ZEPHYR_PACKAGE_WORKSPACE_DIR)
      # Check is done in Zephyr workspace already, thus check only for pure Zephyr candidates.
      get_filename_component(CANDIDATE_DIR ${ZEPHYR_CANDIDATE}/${ZEPHYR_RELATIVE_DIR} ABSOLUTE)
    else()
      get_filename_component(CANDIDATE_DIR ${ZEPHYR_CANDIDATE}/${WORKSPACE_RELATIVE_DIR} ABSOLUTE)
    endif()

    if(CHECK_ZEPHYR_PACKAGE_ZEPHYR_BASE)
        if(CHECK_ZEPHYR_PACKAGE_VERSION_CHECK)
          string(REGEX REPLACE "\.cmake$" "Version.cmake" ZEPHYR_VERSION_CANDIDATE ${ZEPHYR_CANDIDATE})
          include(${ZEPHYR_VERSION_CANDIDATE} NO_POLICY_SCOPE)
          return()
        else()
          include(${ZEPHYR_CANDIDATE} NO_POLICY_SCOPE)
          return()
        endif()
    endif()

    string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "${CANDIDATE_DIR}/" COMMON_INDEX)
    if (COMMON_INDEX EQUAL 0)
      if(CHECK_ZEPHYR_PACKAGE_CHECK_ONLY)
        # A better candidate exists, thus return
        set(PACKAGE_VERSION_COMPATIBLE FALSE)
        return()
      else()
        # A better candidate exists, thus return
        if(CHECK_ZEPHYR_PACKAGE_VERSION_CHECK)
          string(REGEX REPLACE "\.cmake$" "Version.cmake" ZEPHYR_VERSION_CANDIDATE ${ZEPHYR_CANDIDATE})
          include(${ZEPHYR_VERSION_CANDIDATE} NO_POLICY_SCOPE)
          return()
	else()
          include(${ZEPHYR_CANDIDATE} NO_POLICY_SCOPE)
	  return()
        endif()
      endif()
    endif()
  endforeach()
endmacro()
