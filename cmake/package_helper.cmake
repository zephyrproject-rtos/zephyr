# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# The Zephyr package helper script provides a generic script mode interface
# to the Zephyr CMake package and module structure.
#
# The purpose of this script is to provide a single entry for running any Zephyr
# build tool that is executed during CMake configure time without creating a
# complete build system.
#
# It does so by loading the Zephyr CMake modules specified with the 'MODULES'
# argument.
#
# This script executes the given module identical to Zephyr CMake configure time.
# The application source directory must be specified using
# '-S <path-to-sample>'
#
# The build directory will default to current working directory but can be
# controlled with: '-B <path-to-build>'
#
# For example, if you were invoking CMake for 'hello_world' sample as:
#   $ cmake -DBOARD=<board> -B build -S samples/hello_world
#
# you can invoke only dts module (and dependencies) as:
#   $ cmake -DBOARD=<board> -B build -S samples/hello_world \
#           -DMODULES=dts -P <ZEPHYR_BASE>/cmake/package_helper.cmake
#
# It is also possible to pass additional build settings.
# If you invoke CMake for 'hello_world' as:
#
#   $ cmake -DBOARD=<board> -B build -S samples/hello_world -DEXTRA_CONF_FILE=foo.conf
#
# you just add the same argument to the helper like:
#   $ cmake -DBOARD=<board> -B build -S samples/hello_world -DEXTRA_CONF_FILE=foo.conf \
#           -DMODULES=dts -P <ZEPHYR_BASE>/cmake/package_helper.cmake
#
# Note: the samples CMakeLists.txt file is not processed by package helper, so
#       any 'set(<var> <value>)' specified before 'find_package(Zephyr)' must be
#       manually applied, for example if the CMakeLists.txt contains:
#          set(EXTRA_CONF_FILE foo.conf)
#          find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
#       the 'foo.conf' must be specified using '-DEXTRA_CONF_FILE=foo.conf'

cmake_minimum_required(VERSION 3.20.5)

# Find last `-B` and `-S` instances.
foreach(i RANGE ${CMAKE_ARGC})
  if(CMAKE_ARGV${i} MATCHES "^-B(.*)")
    set(argB ${CMAKE_MATCH_1})
    set(argB_index ${i})
  elseif()
  elseif(CMAKE_ARGV${i} MATCHES "^-S(.*)")
    set(argS_index ${i})
    set(argS ${CMAKE_MATCH_1})
  endif()
endforeach()

if(DEFINED argB_index)
  if(DEFINED argB)
    set(CMAKE_BINARY_DIR ${argB})
  else()
    # value of -B follows in next index
    math(EXPR argB_value_index "${argB_index} + 1")
    set(CMAKE_BINARY_DIR ${CMAKE_ARGV${argB_value_index}})
  endif()
endif()

if(DEFINED argS_index)
  if(DEFINED argS)
    set(APPLICATION_SOURCE_DIR ${argS})
  else()
    # value of -S follows in next index
    math(EXPR argS_value_index "${argS_index} + 1")
    set(APPLICATION_SOURCE_DIR ${CMAKE_ARGV${argS_value_index}})
  endif()
endif()

if(NOT DEFINED APPLICATION_SOURCE_DIR)
  message(FATAL_ERROR
    "Source directory not defined, please use '-S <path-to-source>' to the "
    "application for which this tool shall run.\n"
    "For example: -S ${ZEPHYR_BASE}/samples/hello_world"
  )
endif()
if(DEFINED APPLICATION_SOURCE_DIR)
  cmake_path(ABSOLUTE_PATH APPLICATION_SOURCE_DIR NORMALIZE)
endif()

cmake_path(ABSOLUTE_PATH CMAKE_BINARY_DIR NORMALIZE)
set(CMAKE_CURRENT_BINARY_DIR ${CMAKE_BINARY_DIR})

if(NOT DEFINED MODULES)
  message(FATAL_ERROR
    "No MODULES defined, please invoke package helper with minimum one module"
    " to execute in script mode."
  )
endif()

string(REPLACE ";" "," MODULES "${MODULES}")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE} COMPONENTS zephyr_default:${MODULES})
