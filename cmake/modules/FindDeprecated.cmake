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

if("CROSS_COMPILE" IN_LIST Deprecated_FIND_COMPONENTS)
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS CROSS_COMPILE)
  # This code was deprecated after Zephyr v3.1.0
  if(NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT)
    set(ZEPHYR_TOOLCHAIN_VARIANT $ENV{ZEPHYR_TOOLCHAIN_VARIANT})
  endif()

  if(NOT ZEPHYR_TOOLCHAIN_VARIANT AND
     (CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE})))
      set(ZEPHYR_TOOLCHAIN_VARIANT cross-compile CACHE STRING "Zephyr toolchain variant" FORCE)
      message(DEPRECATION  "Setting CROSS_COMPILE without setting ZEPHYR_TOOLCHAIN_VARIANT is deprecated."
                           "Please set ZEPHYR_TOOLCHAIN_VARIANT to 'cross-compile'"
      )
  endif()
endif()

if("SPARSE" IN_LIST Deprecated_FIND_COMPONENTS)
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS SPARSE)
  # This code was deprecated after Zephyr v3.2.0
  if(SPARSE)
    message(DEPRECATION
        "Setting SPARSE='${SPARSE}' is deprecated. "
        "Please set ZEPHYR_SCA_VARIANT to 'sparse'"
    )
    if("${SPARSE}" STREQUAL "y")
      set_ifndef(ZEPHYR_SCA_VARIANT sparse)
    endif()
  endif()
endif()

if("SOURCES" IN_LIST Deprecated_FIND_COMPONENTS)
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS SOURCES)
  if(SOURCES)
    message(DEPRECATION
        "Setting SOURCES prior to calling find_package() for unit tests is deprecated."
        " To add sources after find_package() use:\n"
        "    target_sources(testbinary PRIVATE <source-file.c>)")
  endif()
endif()

if("PYTHON_PREFER" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v3.4.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS PYTHON_PREFER)
  if(DEFINED PYTHON_PREFER)
    message(DEPRECATION "'PYTHON_PREFER' variable is deprecated. Please use "
                        "Python3_EXECUTABLE instead.")
    if(NOT DEFINED Python3_EXECUTABLE)
      set(Python3_EXECUTABLE ${PYTHON_PREFER})
    endif()
  endif()
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

if("SEARCHED_LINKER_SCRIPT" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v3.5.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS SEARCHED_LINKER_SCRIPT)

  # Try a board specific linker file
  set(LINKER_SCRIPT ${BOARD_DIR}/linker.ld)
  if(NOT EXISTS ${LINKER_SCRIPT})
    # If not available, try an SoC specific linker file
    set(LINKER_SCRIPT ${SOC_FULL_DIR}/linker.ld)
  endif()
  message(DEPRECATION
      "Pre-defined `linker.ld` script is deprecated. Please set "
      "BOARD_LINKER_SCRIPT or SOC_LINKER_SCRIPT to point to ${LINKER_SCRIPT} "
      "or one of the Zephyr provided common linker scripts for the ${ARCH} "
      "architecture."
  )
endif()

set(Deprecated_FOUND True)
set(DEPRECATED_FOUND True)
