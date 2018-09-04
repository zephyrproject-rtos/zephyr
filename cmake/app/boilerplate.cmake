# This file must be included into the toplevel CMakeLists.txt file of
# Zephyr applications, e.g. zephyr/samples/hello_world/CMakeLists.txt
# must start with the line:
#
# include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
#
# It exists to reduce boilerplate code that Zephyr expects to be in
# application CMakeLists.txt code.
#
# Omitting it is permitted, but doing so incurs a maintenance cost as
# the application must manage upstream changes to this file.

# app is a CMake library containing all the application code and is
# modified by the entry point ${APPLICATION_SOURCE_DIR}/CMakeLists.txt
# that was specified when cmake was called.

# Determine if we are using MSYS.
#
# We don't use project() because it would take some time to rewrite
# the build scripts to be compatible with everything project() does.
execute_process(
  COMMAND
  uname
  OUTPUT_VARIABLE uname_output
  )
if(uname_output MATCHES "MSYS")
  set(MSYS 1)
endif()

# CMake version 3.8.2 is the real minimum supported version.
#
# Unfortunately CMake requires the toplevel CMakeLists.txt file to
# define the required version, not even invoking it from an included
# file, like boilerplate.cmake, is sufficient. It is however permitted
# to have multiple invocations of cmake_minimum_required.
#
# Under these restraints we use a second 'cmake_minimum_required'
# invocation in every toplevel CMakeLists.txt.
cmake_minimum_required(VERSION 3.8.2)

cmake_policy(SET CMP0002 NEW)

define_property(GLOBAL PROPERTY ZEPHYR_LIBS
    BRIEF_DOCS "Global list of all Zephyr CMake libs that should be linked in"
    FULL_DOCS  "Global list of all Zephyr CMake libs that should be linked in.
zephyr_library() appends libs to this list.")
set_property(GLOBAL PROPERTY ZEPHYR_LIBS "")

define_property(GLOBAL PROPERTY ZEPHYR_INTERFACE_LIBS
    BRIEF_DOCS "Global list of all Zephyr interface libs that should be linked in."
    FULL_DOCS  "Global list of all Zephyr interface libs that should be linked in.
zephyr_interface_library_named() appends libs to this list.")
set_property(GLOBAL PROPERTY ZEPHYR_INTERFACE_LIBS "")

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

set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Application Source Directory")
set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Application Binary Directory")

set(__build_dir ${CMAKE_CURRENT_BINARY_DIR}/zephyr)

set(PROJECT_BINARY_DIR ${__build_dir})

# CMake's 'project' concept has proven to not be very useful for Zephyr
# due in part to how Zephyr is organized and in part to it not fitting well
# with cross compilation.
# CMake therefore tries to rely as little as possible on project()
# and its associated variables, e.g. PROJECT_SOURCE_DIR.
# It is recommended to always use ZEPHYR_BASE instead of PROJECT_SOURCE_DIR
# when trying to reference ENV${ZEPHYR_BASE}.
set(PROJECT_SOURCE_DIR $ENV{ZEPHYR_BASE})

# Convert path to use the '/' separator
string(REPLACE "\\" "/" PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR})

set(ZEPHYR_BINARY_DIR ${PROJECT_BINARY_DIR})
set(ZEPHYR_BASE ${PROJECT_SOURCE_DIR})

set(AUTOCONF_H ${__build_dir}/include/generated/autoconf.h)
# Re-configure (Re-execute all CMakeLists.txt code) when autoconf.h changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})

include(${ZEPHYR_BASE}/cmake/extensions.cmake)

find_package(PythonInterp 3.4)

include(${ZEPHYR_BASE}/cmake/ccache.cmake)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  message(FATAL_ERROR "Source directory equals build directory.\
 In-source builds are not supported.\
 Please specify a build directory, e.g. cmake -Bbuild -H.")
endif()

add_custom_target(
  pristine
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/pristine.cmake
  # Equivalent to rm -rf build/*
  )

# Must be run before kconfig.cmake
if(MSYS)
  execute_process(
    COMMAND
    ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/check_host_is_ok.py
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()
endif()

# The BOARD can be set by 3 sources. Through environment variables,
# through the cmake CLI, and through CMakeLists.txt.
#
# CLI has the highest precedence, then comes environment variables,
# and then finally CMakeLists.txt.
#
# A user can ignore all the precedence rules if he simply always uses
# the same source. E.g. always specifies -DBOARD= on the command line,
# always has an environment variable set, or always has a set(BOARD
# foo) line in his CMakeLists.txt and avoids mixing sources.
#
# The selected BOARD can be accessed through the variable 'BOARD'.

# Read out the cached board value if present
get_property(cached_board_value CACHE BOARD PROPERTY VALUE)

# There are actually 4 sources, the three user input sources, and the
# previously used value (CACHED_BOARD). The previously used value has
# precedence, and if we detect that the user is trying to change the
# value we give him a warning about needing to clean the build
# directory to be able to change boards.

set(board_cli_argument ${cached_board_value}) # Either new or old
if(board_cli_argument STREQUAL CACHED_BOARD)
  # We already have a CACHED_BOARD so there is no new input on the CLI
  unset(board_cli_argument)
endif()

set(board_app_cmake_lists ${BOARD})
if(cached_board_value STREQUAL BOARD)
  # The app build scripts did not set a default, The BOARD we are
  # reading is the cached value from the CLI
  unset(board_app_cmake_lists)
endif()

if(CACHED_BOARD)
  # Warn the user if it looks like he is trying to change the board
  # without cleaning first
  if(board_cli_argument)
    if(NOT (CACHED_BOARD STREQUAL board_cli_argument))
      message(WARNING "The build directory must be cleaned pristinely when changing boards")
      # TODO: Support changing boards without requiring a clean build
    endif()
  endif()

  set(BOARD ${CACHED_BOARD})
elseif(board_cli_argument)
  set(BOARD ${board_cli_argument})

elseif(DEFINED ENV{BOARD})
  set(BOARD $ENV{BOARD})

elseif(board_app_cmake_lists)
  set(BOARD ${board_app_cmake_lists})

else()
  message(FATAL_ERROR "BOARD is not being defined on the CMake command-line in the environment or by the app.")
endif()

assert(BOARD "BOARD not set")
message(STATUS "Selected BOARD ${BOARD}")

# Store the selected board in the cache
set(CACHED_BOARD ${BOARD} CACHE STRING "Selected board")

# Use BOARD to search zephyr/boards/** for a _defconfig file,
# e.g. zephyr/boards/arm/96b_carbon_nrf51/96b_carbon_nrf51_defconfig. When
# found, use that path to infer the ARCH we are building for.
if(NOT BOARD_ROOT)
  set(BOARD_ROOT ${ZEPHYR_BASE})
endif()

if(NOT SOC_ROOT)
  set(SOC_DIR ${ZEPHYR_BASE}/soc)
else()
  set(SOC_DIR ${SOC_ROOT}/soc)
endif()

find_path(BOARD_DIR NAMES "${BOARD}_defconfig" PATHS ${BOARD_ROOT}/boards/*/* NO_DEFAULT_PATH)

assert_with_usage(BOARD_DIR "No board named '${BOARD}' found")

get_filename_component(BOARD_ARCH_DIR ${BOARD_DIR} DIRECTORY)
get_filename_component(ARCH ${BOARD_ARCH_DIR} NAME)
get_filename_component(BOARD_FAMILY ${BOARD_DIR} NAME)

if(CONF_FILE)
  # CONF_FILE has either been specified on the cmake CLI or is already
  # in the CMakeCache.txt. This has precedence over the environment
  # variable CONF_FILE and the default prj.conf
elseif(DEFINED ENV{CONF_FILE})
  set(CONF_FILE $ENV{CONF_FILE})

elseif(COMMAND set_conf_file)
  set_conf_file()

elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)

elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/prj.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj.conf)
endif()

set(CONF_FILE ${CONF_FILE} CACHE STRING "If desired, you can build the application using\
the configuration settings specified in an alternate .conf file using this parameter. \
These settings will override the settings in the application’s .config file or its default .conf file.\
Multiple files may be listed, e.g. CONF_FILE=\"prj1.conf prj2.conf\"")

if(DTC_OVERLAY_FILE)
  # DTC_OVERLAY_FILE has either been specified on the cmake CLI or is already
  # in the CMakeCache.txt. This has precedence over the environment
  # variable DTC_OVERLAY_FILE
elseif(DEFINED ENV{DTC_OVERLAY_FILE})
  set(DTC_OVERLAY_FILE $ENV{DTC_OVERLAY_FILE})
elseif(EXISTS          ${APPLICATION_SOURCE_DIR}/${BOARD}.overlay)
  set(DTC_OVERLAY_FILE ${APPLICATION_SOURCE_DIR}/${BOARD}.overlay)
endif()

set(DTC_OVERLAY_FILE ${DTC_OVERLAY_FILE} CACHE STRING "If desired, you can \
build the application using the DT configuration settings specified in an \
alternate .overlay file using this parameter. These settings will override the \
settings in the board's .dts file. Multiple files may be listed, e.g. \
DTC_OVERLAY_FILE=\"dts1.overlay dts2.overlay\"")

# Prevent CMake from testing the toolchain
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

include(${ZEPHYR_BASE}/cmake/version.cmake)
include(${ZEPHYR_BASE}/cmake/host-tools.cmake)
include(${ZEPHYR_BASE}/cmake/kconfig.cmake)
include(${ZEPHYR_BASE}/cmake/toolchain.cmake)

find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} describe
    WORKING_DIRECTORY ${ZEPHYR_BASE}
    OUTPUT_VARIABLE BUILD_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

set(SOC_NAME ${CONFIG_SOC})
set(SOC_SERIES ${CONFIG_SOC_SERIES})
set(SOC_FAMILY ${CONFIG_SOC_FAMILY})

if("${SOC_SERIES}" STREQUAL "")
  set(SOC_PATH ${SOC_NAME})
else()
  set(SOC_PATH ${SOC_FAMILY}/${SOC_SERIES})
endif()


# DTS should be run directly after kconfig because CONFIG_ variables
# from kconfig and dts should be available at the same time. But
# running DTS involves running the preprocessor, so we put it behind
# toolchain. Meaning toolchain.cmake is the only component where
# kconfig and dts variables aren't available at the same time.
include(${ZEPHYR_BASE}/cmake/dts.cmake)

set(KERNEL_NAME ${CONFIG_KERNEL_BIN_NAME})

set(KERNEL_ELF_NAME   ${KERNEL_NAME}.elf)
set(KERNEL_BIN_NAME   ${KERNEL_NAME}.bin)
set(KERNEL_HEX_NAME   ${KERNEL_NAME}.hex)
set(KERNEL_MAP_NAME   ${KERNEL_NAME}.map)
set(KERNEL_LST_NAME   ${KERNEL_NAME}.lst)
set(KERNEL_S19_NAME   ${KERNEL_NAME}.s19)
set(KERNEL_EXE_NAME   ${KERNEL_NAME}.exe)
set(KERNEL_STAT_NAME  ${KERNEL_NAME}.stat)
set(KERNEL_STRIP_NAME ${KERNEL_NAME}.strip)

# Populate USER_CACHE_DIR with a directory that user applications may
# write cache files to.
if(NOT DEFINED USER_CACHE_DIR)
  find_appropriate_cache_directory(USER_CACHE_DIR)
endif()
message(STATUS "Cache files will be written to: ${USER_CACHE_DIR}")

include(${BOARD_DIR}/board.cmake OPTIONAL)

zephyr_library_named(app)

add_subdirectory(${ZEPHYR_BASE} ${__build_dir})

# Link 'app' with the Zephyr interface libraries.
#
# NB: This must be done in boilerplate.cmake because 'app' can only be
# modified in the CMakeLists.txt file that created it. And it must be
# done after 'add_subdirectory(${ZEPHYR_BASE} ${__build_dir})'
# because interface libraries are defined while processing that
# subdirectory.
get_property(ZEPHYR_INTERFACE_LIBS_PROPERTY GLOBAL PROPERTY ZEPHYR_INTERFACE_LIBS)
foreach(boilerplate_lib ${ZEPHYR_INTERFACE_LIBS_PROPERTY})
  # Linking 'app' with 'boilerplate_lib' causes 'app' to inherit the INTERFACE
  # properties of 'boilerplate_lib'. The most common property is 'include
  # directories', but it is also possible to have defines and compiler
  # flags in the interface of a library.
  #
  string(TOUPPER ${boilerplate_lib} boilerplate_lib_upper_case) # Support lowercase lib names
  target_link_libraries_ifdef(
    CONFIG_APP_LINK_WITH_${boilerplate_lib_upper_case}
    app
    PUBLIC
    ${boilerplate_lib}
    )
endforeach()


if(NOT EXISTS ${ZEPHYR_BASE}/hide-defaults-note)
    message(STATUS "\n\
*******************************\n\
*** NOTE TO KCONFIG AUTHORS ***\n\
*******************************\n\
\n\
The behavior of Kconfig 'default' properties in Zephyr has changed. The \n\
earliest default with a satisfied condition is now used, instead of the \n\
last one. This is standard Kconfig behavior.\n\
\n\
See http://docs.zephyrproject.org/latest/porting/board_porting.html#old-zephyr-kconfig-behavior-for-defaults.\n\
\n\
To get rid of this note, create a file called 'hide-defaults-note' in the \n\
Zephyr root directory. An empty file is fine.")
endif()
