# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# This CMake module will load all Zephyr CMake modules in correct order for
# default Zephyr build system.
#
# Outcome:
# See individual CMake module descriptions

include_guard(GLOBAL)

# The code line below defines the real minimum supported CMake version.
#
# Unfortunately CMake requires the toplevel CMakeLists.txt file to define the
# required version, not even invoking it from a CMake module is sufficient.
# It is however permitted to have multiple invocations of cmake_minimum_required.
cmake_minimum_required(VERSION 3.20.0)

message(STATUS "Application: ${APPLICATION_SOURCE_DIR}")

# Different CMake versions can have very subtle differences, for
# instance CMake 3.21 links object files in a different order compared
# to CMake 3.20; this produces different binaries.
message(STATUS "CMake version: ${CMAKE_VERSION}")

# Find and execute workspace build configuration
find_package(ZephyrBuildConfiguration
  QUIET NO_POLICY_SCOPE
  NAMES ZephyrBuild
  PATHS ${ZEPHYR_BASE}/../*
  NO_CMAKE_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  NO_CMAKE_PACKAGE_REGISTRY
  NO_CMAKE_SYSTEM_PATH
  NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
)

# Find and execute application-specific build configuration
find_package(ZephyrAppConfiguration
  QUIET NO_POLICY_SCOPE
  NAMES ZephyrApp
  PATHS ${APPLICATION_SOURCE_DIR}
  NO_CMAKE_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  NO_CMAKE_PACKAGE_REGISTRY
  NO_CMAKE_SYSTEM_PATH
  NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
)

# Test and error-out if we are affected by the PyPI CMake 3.22.1 / 3.22.2 bug
if(${CMAKE_VERSION} VERSION_EQUAL 3.22.1 OR ${CMAKE_VERSION} VERSION_EQUAL 3.22.2)
  # It seems only pip-installed builds are affected so we test to see if we are affected
  cmake_path(GET ZEPHYR_BASE PARENT_PATH test_cmake_path)
  if(ZEPHYR_BASE STREQUAL test_cmake_path)
    message(FATAL_ERROR "The CMake version ${CMAKE_VERSION} installed suffers"
            " the \n 'cmake_path(... PARENT_PATH)' bug, see: \n"
	    "https://gitlab.kitware.com/cmake/cmake/-/issues/23187\n"
	    "https://github.com/scikit-build/cmake-python-distributions/issues/221\n"
	    "Please install another CMake version or use a build of CMake that"
	    " does not come from PyPI."
    )
  endif()
endif()

# Prepare user cache
list(APPEND zephyr_cmake_modules python)
list(APPEND zephyr_cmake_modules user_cache)

# Load Zephyr extensions
list(APPEND zephyr_cmake_modules extensions)
list(APPEND zephyr_cmake_modules version)

# Load basic settings
list(APPEND zephyr_cmake_modules basic_settings)

#
# Find tools
#

list(APPEND zephyr_cmake_modules west)
list(APPEND zephyr_cmake_modules ccache)

# Load default root settings
list(APPEND zephyr_cmake_modules root)

#
# Find Zephyr modules.
# Those may contain additional DTS, BOARD, SOC, ARCH ROOTs.
# Also create the Kconfig binary dir for generated Kconf files.
#
list(APPEND zephyr_cmake_modules zephyr_module)

list(APPEND zephyr_cmake_modules boards)
list(APPEND zephyr_cmake_modules shields)
list(APPEND zephyr_cmake_modules snippets)
list(APPEND zephyr_cmake_modules arch_v1)
list(APPEND zephyr_cmake_modules hwm_v2)
list(APPEND zephyr_cmake_modules configuration_files)
list(APPEND zephyr_cmake_modules generated_file_directories)

# Include board specific device-tree flags before parsing.
set(pre_dt_board "\${BOARD_DIR}/pre_dt_board.cmake" OPTIONAL)
list(APPEND zephyr_cmake_modules "\${pre_dt_board}")

# DTS should be close to kconfig because CONFIG_ variables from
# kconfig and dts should be available at the same time.
list(APPEND zephyr_cmake_modules dts)
list(APPEND zephyr_cmake_modules kconfig)
list(APPEND zephyr_cmake_modules arch_v2)
list(APPEND zephyr_cmake_modules soc_v1)
list(APPEND zephyr_cmake_modules soc_v2)
list(APPEND zephyr_cmake_modules rust)

foreach(component ${SUB_COMPONENTS})
  if(NOT ${component} IN_LIST zephyr_cmake_modules)
    message(FATAL_ERROR
      "Subcomponent '${component}' not default module for Zephyr CMake build system.\n"
      "Please choose one or more valid components: ${zephyr_cmake_modules}"
    )
  endif()
endforeach()

foreach(module IN LISTS zephyr_cmake_modules)
  # Ensures any module of type `${module}` are properly expanded to list before
  # passed on the `include(${module})`.
  # This is done twice to support cases where the content of `${module}` itself
  # contains a variable, like `${BOARD_DIR}`.
  string(CONFIGURE "${module}" module)
  string(CONFIGURE "${module}" module)
  include(${module})

  list(REMOVE_ITEM SUB_COMPONENTS ${module})
  if(DEFINED SUB_COMPONENTS AND NOT SUB_COMPONENTS)
    # All requested Zephyr CMake modules have been loaded, so let's return.
    return()
  endif()
endforeach()

include(kernel)
