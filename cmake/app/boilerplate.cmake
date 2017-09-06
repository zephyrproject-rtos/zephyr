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

include($ENV{ZEPHYR_BASE}/cmake/extensions.cmake)

find_package(PythonInterp 3.4)

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

include(    ${PROJECT_SOURCE_DIR}/defaults.cmake       OPTIONAL)
include(${APPLICATION_SOURCE_DIR}/defaults.cmake       OPTIONAL)

include(    ${PROJECT_SOURCE_DIR}/local-defaults.cmake OPTIONAL)
include(${APPLICATION_SOURCE_DIR}/local-defaults.cmake OPTIONAL)

if(EXISTS ${CONF_FILE__for__${BOARD}})
  set(CONF_FILE ${CONF_FILE__for__${BOARD}})
elseif(NOT CONF_FILE)
  if(EXISTS ${APPLICATION_SOURCE_DIR}/prj.conf)
    set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj.conf)
  endif()
endif()

set(PREBUILT_HOST_TOOLS     ${PREBUILT_HOST_TOOLS}    CACHE PATH   "")
set(ZEPHYR_SDK_INSTALL_DIR  ${ZEPHYR_SDK_INSTALL_DIR} CACHE PATH   "Zephyr SDK install directory")
set(ZEPHYR_GCC_VARIANT      ${ZEPHYR_GCC_VARIANT}     CACHE STRING "Zephyr GCC variant")
set(BOARD                   ${BOARD}                  CACHE STRING "Board")
set(CONF_FILE               ${CONF_FILE}              CACHE STRING "If desired, you can build the application using\
the configuration settings specified in an alternate .conf file using this parameter. \
These settings will override the settings in the application’s .config file or its default .conf file.\
Multiple files may be listed, e.g. CONF_FILE=\"prj1.conf prj2.conf\"")

assert(ZEPHYR_GCC_VARIANT "ZEPHYR_GCC_VARIANT not set. Please configure zephyr/local-defaults.cmake")
assert(BOARD              "This should be nearly impossible since BOARD is configured in zephyr/defaults.cmake")

message(STATUS "Selected BOARD ${BOARD}")

if(PREBUILT_HOST_TOOLS)
  set(KCONFIG_CONF  ${PREBUILT_HOST_TOOLS}/kconfig/conf)
  set(KCONFIG_MCONF ${PREBUILT_HOST_TOOLS}/kconfig/mconf)
endif()

# Use BOARD to search zephyr/boards/** for a _defconfig file,
# e.g. zephyr/boards/arm/96b_carbon_nrf51/96b_carbon_nrf51_defconfig. When
# found, use that path to infer the ARCH we are building for.
unset(BOARD_DIR CACHE)
find_path(BOARD_DIR NAMES "${BOARD}_defconfig" PATHS $ENV{ZEPHYR_BASE}/boards/*/* NO_DEFAULT_PATH)
assert(BOARD_DIR "No board named '${BOARD}' found")

get_filename_component(BOARD_ARCH_DIR ${BOARD_DIR} DIRECTORY)
get_filename_component(ARCH ${BOARD_ARCH_DIR} NAME)

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
include($ENV{ZEPHYR_BASE}/cmake/host-tools-${ZEPHYR_GCC_VARIANT}.cmake)

assert(KCONFIG_CONF "PREBUILT_HOST_TOOLS was not set. Please follow the Zephyr installation instructions.")

include($ENV{ZEPHYR_BASE}/cmake/kconfig.cmake)
include($ENV{ZEPHYR_BASE}/cmake/toolchain-${ZEPHYR_GCC_VARIANT}.cmake)

include(${BOARD_DIR}/board.cmake OPTIONAL)

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
