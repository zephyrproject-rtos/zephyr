# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(number_crunching)

# defines targets and sources
target_sources(app PRIVATE
  src/main.c
  src/math_ops.c
)
zephyr_include_directories(include)

if(DEFINED ENV{LIB_LOCATION})
  message(STATUS "LIB_LOCATION environment variable defined")

  # contains a "proprietary" library we will link to
  # this should set the INCLUDE_DIR, LIB_DIR and LIB_NAME variables
  add_subdirectory($ENV{LIB_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/proprietary)

  # this is an example for NatureDSP backend
  target_sources(app PRIVATE
    src/nature_dsp_wrapper.c
  )

  if(INCLUDE_DIR)
    zephyr_include_directories($ENV{LIB_LOCATION}/${INCLUDE_DIR})
  endif()

  if(LIB_DIR AND LIB_NAME)
    zephyr_link_libraries($ENV{LIB_LOCATION}/${LIB_DIR}/${LIB_NAME})
  endif()
else()
  message(STATUS "LIB_LOCATION environment variable NOT defined")
  # this is an example for CMSIS-DSP backend
  target_sources(app PRIVATE
    src/cmsis_dsp_wrapper.c
  )
endif()
