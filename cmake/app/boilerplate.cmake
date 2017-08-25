cmake_minimum_required(VERSION 3.7)
cmake_policy(SET CMP0000 OLD)
cmake_policy(SET CMP0002 NEW)

# This file must be included into the toplevel CMakeLists.txt file of
# Zephyr applications, e.g. zephyr/samples/hello_world/CMakeLists.txt
# must start with the line:
#
# include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake)
#
# It exists to reduce boilerplate code that Zephyr expects to be in
# application CMakeLists.txt code.
#
# Omitting it is permitted, but doing so incurs a maintenance cost as
# the application must manage upstream changes to this file.

# app is a CMake library containing all the application code and is
# modified by the entry point ${APPLICATION_SOURCE_DIR}/CMakeLists.txt
# that was specified when cmake was called.

set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Application Source Directory")
set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Application Binary Directory")

set(__build_dir ${CMAKE_CURRENT_BINARY_DIR}/zephyr)

set(PROJECT_BINARY_DIR ${__build_dir})
set(PROJECT_SOURCE_DIR $ENV{ZEPHYR_BASE})

set(AUTOCONF_H ${__build_dir}/include/generated/autoconf.h)
# Re-configure (Re-execute all CMakeLists.txt code) when autoconf.h changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})

find_package(PythonInterp 3.4)

if(NOT PREBUILT_HOST_TOOLS)
  set(PREBUILT_HOST_TOOLS $ENV{PREBUILT_HOST_TOOLS} CACHE PATH "")
  if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    set(PREBUILT_HOST_TOOLS $ENV{ZEPHYR_BASE}/scripts/prebuilt)
  endif()
endif()

if(NOT PREBUILT_HOST_TOOLS)
  message(FATAL_ERROR "PREBUILT_HOST_TOOLS was not set. Please follow the Zephyr installation instructions.")
endif()

if(NOT ZEPHYR_GCC_VARIANT)
  set(ZEPHYR_GCC_VARIANT $ENV{ZEPHYR_GCC_VARIANT})
endif()
set(ZEPHYR_GCC_VARIANT ${ZEPHYR_GCC_VARIANT} CACHE STRING "Zephyr GCC variant")
if(NOT ZEPHYR_GCC_VARIANT)
  message(FATAL_ERROR "ZEPHYR_GCC_VARIANT not set")
endif()

if(NOT BOARD)
  set(BOARD $ENV{BOARD})
endif()
set(BOARD ${BOARD} CACHE STRING "Board")
if(NOT BOARD)
  message(FATAL_ERROR "BOARD not set")
endif()

find_path(BOARD_DIR NAMES ${BOARD} PATHS $ENV{ZEPHYR_BASE}/boards/* NO_DEFAULT_PATH)
if(NOT BOARD_DIR)
  message(FATAL_ERROR "No board named '${BOARD}' found")
endif()

get_filename_component(ARCH ${BOARD_DIR} NAME)

if(CONF_FILE)
  # CONF_FILE has either been specified on the cmake CLI or is already
  # in the CMakeCache.txt. This has precedence over the environment
  # variable CONF_FILE and the default prj.conf
elseif(DEFINED ENV{CONF_FILE})
  set(CONF_FILE $ENV{CONF_FILE})
elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/prj.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj.conf)
endif()

set(CONF_FILE ${CONF_FILE} CACHE STRING "If desired, you can build the application using\
the configuration settings specified in an alternate .conf file using this parameter. \
These settings will override the settings in the application’s .config file or its default .conf file.\
Multiple files may be listed, e.g. CONF_FILE=\"prj1.conf prj2.conf\"")

# Prevent CMake from testing the toolchain
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

# Determine if the application is in zephyr/tests, if so it is a test
# and must be built differently
string(REGEX MATCH "^$ENV{ZEPHYR_BASE}/tests" match ${APPLICATION_SOURCE_DIR})
if(match)
  set(IS_TEST 1)
endif()

include($ENV{ZEPHYR_BASE}/cmake/version.cmake)
include($ENV{ZEPHYR_BASE}/cmake/extensions.cmake)
include($ENV{ZEPHYR_BASE}/cmake/kconfig.cmake)
include($ENV{ZEPHYR_BASE}/cmake/toolchain-${ZEPHYR_GCC_VARIANT}.cmake)
include($ENV{ZEPHYR_BASE}/boards/${ARCH}/${BOARD}/board.cmake OPTIONAL)

zephyr_library_named(app)

add_subdirectory($ENV{ZEPHYR_BASE} ${__build_dir})

define_property(GLOBAL PROPERTY ZEPHYR_LIBS
    BRIEF_DOCS "Global list of all Zephyr CMake libs that should be linked in"
    FULL_DOCS "Global list of all Zephyr CMake libs that should be linked in. zephyr_library() appends libs to this list.")
set_property(GLOBAL PROPERTY ZEPHYR_LIBS "")

define_property(GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES
  BRIEF_DOCS "Object files that are generated after Zephyr has been linked once."
  FULL_DOCS "\
Object files that are generated after Zephyr has been linked once.\
May include mmu tables, etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES "")

define_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES
  BRIEF_DOCS "Source files that are generated after Zephyr has been linked once."
  FULL_DOCS "\
Object files that are generated after Zephyr has been linked once.\
May include isr_tables.c etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES "")
