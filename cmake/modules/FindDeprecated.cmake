# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

# FindDeprecated module provides a single location for deprecated CMake build code.
# Whenever CMake code is deprecated it should be moved to this module and
# corresponding COMPONENTS should be created with name identifying the deprecated code.
#
# This makes it easier to maintain deprecated code and cleanup such code when it
# has been deprecated for two releases.
#
# Example:
# CMakeList.txt contains deprecated code, like:
# if(DEPRECATED_VAR)
#   deprecated()
# endif()
#
# such code can easily be around for a long time, so therefore such code should
# be moved to this module and can then be loaded as:
# FindDeprecated.cmake
# if(<deprecated_name> IN_LIST Deprecated_FIND_COMPONENTS)
#   # This code has been deprecated after Zephyr x.y
#   if(DEPRECATED_VAR)
#     deprecated()
#   endif()
# endif()
#
# and then in the original CMakeLists.txt, this code is inserted instead:
# find_package(Deprecated COMPONENTS <deprecated_name>)
#
# The module defines the following variables:
#
# 'Deprecated_FOUND', 'DEPRECATED_FOUND'
# True if the Deprecated component was found and loaded.

if("${Deprecated_FIND_COMPONENTS}" STREQUAL "")
  message(WARNING "find_package(Deprecated) missing required COMPONENTS keyword")
endif()

if("soc_vars" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code has been deprecated after Zephyr 4.4
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS soc_vars)
  set(SOC_NAME   ${CONFIG_SOC})
  set(SOC_SERIES ${CONFIG_SOC_SERIES})
  set(SOC_FAMILY ${CONFIG_SOC_FAMILY})
  set(SOC_V2_DIR ${SOC_FULL_DIR})

  # Adds a watch for these deprecated variables to emit a warning
  variable_watch(SOC_NAME deprecated_soc_var)
  variable_watch(SOC_SERIES deprecated_soc_var)
  variable_watch(SOC_FAMILY deprecated_soc_var)
  variable_watch(SOC_V2_DIR deprecated_soc_var)
endif()

if("ENV_CLANG_ROOT_DIR" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code has been deprecated after Zephyr 4.5
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS ENV_CLANG_ROOT_DIR)

  if(DEFINED ENV{CLANG_ROOT_DIR})
    message(DEPRECATION "CLANG_ROOT_DIR is deprecated, set LLVM_TOOLCHAIN_PATH instead.")
    set_ifndef(LLVM_TOOLCHAIN_PATH "$ENV{CLANG_ROOT_DIR}")
  endif()
endif()

if("CLANG_ROOT_DIR" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code has been deprecated after Zephyr 4.5
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS CLANG_ROOT_DIR)

  if(DEFINED CLANG_ROOT_DIR)
    message(DEPRECATION "CLANG_ROOT_DIR is deprecated, set LLVM_TOOLCHAIN_PATH instead.")
    set(LLVM_TOOLCHAIN_PATH ${CLANG_ROOT_DIR} CACHE PATH "clang install directory")
  endif()
endif()

if(NOT "${Deprecated_FIND_COMPONENTS}" STREQUAL "")
  message(STATUS "The following deprecated component(s) could not be found: "
    "${Deprecated_FIND_COMPONENTS}")
endif()

set(Deprecated_FOUND True)
set(DEPRECATED_FOUND True)
