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

# CMake version 3.8.2 is the only supported version. Version 3.9 is
# not supported because it introduced a warning that we do not wish to
# show to users. Specifically, it displays a warning when an OLD
# policy is used, but we need policy CMP0000 set to OLD to avoid
# copy-pasting cmake_minimum_required across application
# CMakeLists.txt files.
cmake_minimum_required(VERSION 3.8.2)
cmake_policy(SET CMP0000 OLD)
cmake_policy(SET CMP0002 NEW)

set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Application Source Directory")
set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Application Binary Directory")

set(__build_dir ${CMAKE_CURRENT_BINARY_DIR}/zephyr)

set(PROJECT_BINARY_DIR ${__build_dir})
set(PROJECT_SOURCE_DIR $ENV{ZEPHYR_BASE})

set(ZEPHYR_BINARY_DIR ${PROJECT_BINARY_DIR})
set(ZEPHYR_SOURCE_DIR ${PROJECT_SOURCE_DIR})

set(AUTOCONF_H ${__build_dir}/include/generated/autoconf.h)
# Re-configure (Re-execute all CMakeLists.txt code) when autoconf.h changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})

include($ENV{ZEPHYR_BASE}/cmake/extensions.cmake)

find_package(PythonInterp 3.4)

# Generate syscall_macros.h at configure-time because it has virtually
# no dependencies
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include/generated)
execute_process_safely(
  COMMAND
  ${PYTHON_EXECUTABLE}
  $ENV{ZEPHYR_BASE}/scripts/gen_syscall_header.py
  OUTPUT_FILE ${ZEPHYR_BINARY_DIR}/include/generated/syscall_macros.h
  )

if(NOT PREBUILT_HOST_TOOLS)
  set(PREBUILT_HOST_TOOLS $ENV{PREBUILT_HOST_TOOLS} CACHE PATH "")
  if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    set(PREBUILT_HOST_TOOLS $ENV{ZEPHYR_BASE}/scripts/prebuilt)
  endif()
endif()

if(NOT ZEPHYR_GCC_VARIANT)
  set(ZEPHYR_GCC_VARIANT $ENV{ZEPHYR_GCC_VARIANT})
endif()
set(ZEPHYR_GCC_VARIANT ${ZEPHYR_GCC_VARIANT} CACHE STRING "Zephyr GCC variant")
if(NOT ZEPHYR_GCC_VARIANT)
  message(FATAL_ERROR "ZEPHYR_GCC_VARIANT not set")
endif()

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  message(FATAL_ERROR "Source directory equals build directory.\
 In-source builds are not supported.\
 Please specify a build directory, e.g. cmake -Bbuild -H.")
endif()

add_custom_target(
  pristine
  COMMAND ${CMAKE_COMMAND} -P $ENV{ZEPHYR_BASE}/cmake/pristine.cmake
  # Equivalent to rm -rf build/*
  )

if(NOT BOARD)
  set(BOARD $ENV{BOARD})
endif()
set(BOARD ${BOARD} CACHE STRING "Board")
if(NOT BOARD)
  message(FATAL_ERROR "BOARD not set")
endif()
message(STATUS "Selected BOARD ${BOARD}")

# Use BOARD to search zephyr/boards/** for a _defconfig file,
# e.g. zephyr/boards/arm/96b_carbon_nrf51/96b_carbon_nrf51_defconfig. When
# found, use that path to infer the ARCH we are building for.
find_path(BOARD_DIR NAMES "${BOARD}_defconfig" PATHS $ENV{ZEPHYR_BASE}/boards/*/* NO_DEFAULT_PATH)
assert(BOARD_DIR "No board named '${BOARD}' found")

get_filename_component(BOARD_ARCH_DIR ${BOARD_DIR} DIRECTORY)
get_filename_component(ARCH ${BOARD_ARCH_DIR} NAME)
get_filename_component(BOARD_FAMILY ${BOARD_DIR} NAME)

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
include($ENV{ZEPHYR_BASE}/cmake/host-tools.cmake)
include($ENV{ZEPHYR_BASE}/cmake/kconfig.cmake)
include($ENV{ZEPHYR_BASE}/cmake/toolchain.cmake)

set(KERNEL_NAME ${CONFIG_KERNEL_BIN_NAME})

set(KERNEL_ELF_NAME   ${KERNEL_NAME}.elf)
set(KERNEL_BIN_NAME   ${KERNEL_NAME}.bin)
set(KERNEL_HEX_NAME   ${KERNEL_NAME}.hex)
set(KERNEL_MAP_NAME   ${KERNEL_NAME}.map)
set(KERNEL_LST_NAME   ${KERNEL_NAME}.lst)
set(KERNEL_S19_NAME   ${KERNEL_NAME}.s19)
set(KERNEL_STAT_NAME  ${KERNEL_NAME}.stat)
set(KERNEL_STRIP_NAME ${KERNEL_NAME}.strip)

include(${BOARD_DIR}/board.cmake OPTIONAL)

zephyr_library_named(app)

execute_process_safely(
  COMMAND
  ${PYTHON_EXECUTABLE}
  $ENV{ZEPHYR_BASE}/scripts/gen_syscalls.py
  --include          $ENV{ZEPHYR_BASE}/include            # Read files from this dir
  --base-output      include/generated/syscalls           # Write to this dir
  --syscall-dispatch include/generated/syscall_dispatch.c # Write this file
  INPUT_FILE         kconfig/include/config/auto.conf     # Read this file from stdin
  OUTPUT_FILE        include/generated/syscall_list.h     # Write stdout to this file
  WORKING_DIRECTORY ${ZEPHYR_BINARY_DIR}
  )

add_subdirectory($ENV{ZEPHYR_BASE} ${__build_dir})

define_property(GLOBAL PROPERTY ZEPHYR_LIBS
    BRIEF_DOCS "Global list of all Zephyr CMake libs that should be linked in"
    FULL_DOCS  "Global list of all Zephyr CMake libs that should be linked in. zephyr_library() appends libs to this list.")
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

define_property(GLOBAL PROPERTY FLASH_SCRIPT_ENV_VARS
  BRIEF_DOCS "Environment variables that should be passed to FLASH_SCRIPT"
  FULL_DOCS  "Environment variables that should be passed to FLASH_SCRIPT"
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES "")
