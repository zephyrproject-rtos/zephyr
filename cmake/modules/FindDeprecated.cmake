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

if("toolchain_ld_base" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v4.0.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS toolchain_ld_base)

  if(COMMAND toolchain_ld_base)
    message(DEPRECATION
        "The macro/function 'toolchain_ld_base' is deprecated. "
        "Please use '${LINKER}/linker_flags.cmake' and define the appropriate "
        "linker flags as properties instead. "
        "See '${ZEPHYR_BASE}/cmake/linker/linker_flags_template.cmake' for "
        "known linker properties."
    )
    toolchain_ld_base()
  endif()
endif()

if("toolchain_ld_baremetal" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v4.0.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS toolchain_ld_baremetal)

  if(COMMAND toolchain_ld_baremetal)
    message(DEPRECATION
        "The macro/function 'toolchain_ld_baremetal' is deprecated. "
        "Please use '${LINKER}/linker_flags.cmake' and define the appropriate "
        "linker flags as properties instead. "
        "See '${ZEPHYR_BASE}/cmake/linker/linker_flags_template.cmake' for "
        "known linker properties."
    )
    toolchain_ld_baremetal()
  endif()
endif()

if("toolchain_ld_cpp" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v4.0.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS toolchain_ld_cpp)

  if(COMMAND toolchain_ld_cpp)
    message(DEPRECATION
        "The macro/function 'toolchain_ld_cpp' is deprecated. "
        "Please use '${LINKER}/linker_flags.cmake' and define the appropriate "
        "linker flags as properties instead. "
        "See '${ZEPHYR_BASE}/cmake/linker/linker_flags_template.cmake' for "
        "known linker properties."
    )
    toolchain_ld_cpp()
  endif()
endif()

if(NOT "${Deprecated_FIND_COMPONENTS}" STREQUAL "")
  message(STATUS "The following deprecated component(s) could not be found: "
                 "${Deprecated_FIND_COMPONENTS}")
endif()

set(Deprecated_FOUND True)
set(DEPRECATED_FOUND True)
