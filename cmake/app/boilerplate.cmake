# SPDX-License-Identifier: Apache-2.0

# This file must be included into the toplevel CMakeLists.txt file of
# Zephyr applications.
# Zephyr CMake package automatically includes this file when CMake function
# find_package() is used.
#
# To ensure this file is loaded in a Zephyr application it must start with
# one of those lines:
#
# find_package(Zephyr)
# find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
#
# The `REQUIRED HINTS $ENV{ZEPHYR_BASE}` variant is required for any application
# inside the Zephyr repository.
#
# It exists to reduce boilerplate code that Zephyr expects to be in
# application CMakeLists.txt code.

# CMake version 3.13.1 is the real minimum supported version.
#
# Unfortunately CMake requires the toplevel CMakeLists.txt file to
# define the required version, not even invoking it from an included
# file, like boilerplate.cmake, is sufficient. It is however permitted
# to have multiple invocations of cmake_minimum_required.
#
# Under these restraints we use a second 'cmake_minimum_required'
# invocation in every toplevel CMakeLists.txt.
cmake_minimum_required(VERSION 3.13.1)

# CMP0002: "Logical target names must be globally unique"
cmake_policy(SET CMP0002 NEW)

# Use the old CMake behaviour until the build scripts have been ported
# to the new behaviour.
# CMP0079: "target_link_libraries() allows use with targets in other directories"
cmake_policy(SET CMP0079 OLD)

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
Source files that are generated after Zephyr has been linked once.\
May include isr_tables.c etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES "")

set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Application Source Directory")
set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Application Binary Directory")

set(__build_dir ${CMAKE_CURRENT_BINARY_DIR}/zephyr)

set(PROJECT_BINARY_DIR ${__build_dir})

message(STATUS "Application: ${APPLICATION_SOURCE_DIR}")

add_custom_target(code_data_relocation_target)

# CMake's 'project' concept has proven to not be very useful for Zephyr
# due in part to how Zephyr is organized and in part to it not fitting well
# with cross compilation.
# Zephyr therefore tries to rely as little as possible on project()
# and its associated variables, e.g. PROJECT_SOURCE_DIR.
# It is recommended to always use ZEPHYR_BASE instead of PROJECT_SOURCE_DIR
# when trying to reference ENV${ZEPHYR_BASE}.

set(ENV_ZEPHYR_BASE $ENV{ZEPHYR_BASE})
# This add support for old style boilerplate include.
if((NOT DEFINED ZEPHYR_BASE) AND (DEFINED ENV_ZEPHYR_BASE))
  set(ZEPHYR_BASE ${ENV_ZEPHYR_BASE} CACHE PATH "Zephyr base")
endif()

find_package(ZephyrBuildConfiguration NAMES ZephyrBuild PATHS ${ZEPHYR_BASE}/../* QUIET NO_DEFAULT_PATH NO_POLICY_SCOPE)

# Note any later project() resets PROJECT_SOURCE_DIR
file(TO_CMAKE_PATH "${ZEPHYR_BASE}" PROJECT_SOURCE_DIR)

set(ZEPHYR_BINARY_DIR ${PROJECT_BINARY_DIR})

set(AUTOCONF_H ${__build_dir}/include/generated/autoconf.h)
# Re-configure (Re-execute all CMakeLists.txt code) when autoconf.h changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})


#
# Import more CMake functions and macros
#

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(${ZEPHYR_BASE}/cmake/extensions.cmake)
include(${ZEPHYR_BASE}/cmake/version.cmake)  # depends on hex.cmake

#
# Find tools
#

include(${ZEPHYR_BASE}/cmake/python.cmake)
include(${ZEPHYR_BASE}/cmake/west.cmake)
include(${ZEPHYR_BASE}/cmake/git.cmake)  # depends on version.cmake
include(${ZEPHYR_BASE}/cmake/ccache.cmake)

#
# Find Zephyr modules.
# Those may contain additional DTS, BOARD, SOC, ARCH ROOTs.
# Also create the Kconfig binary dir for generated Kconf files.
#
set(KCONFIG_BINARY_DIR ${CMAKE_BINARY_DIR}/Kconfig)
file(MAKE_DIRECTORY ${KCONFIG_BINARY_DIR})
include(${ZEPHYR_BASE}/cmake/zephyr_module.cmake)

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

# Dummy add to generate files.
zephyr_linker_sources(SECTIONS)

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
    if(NOT ((CACHED_BOARD STREQUAL board_cli_argument) OR (BOARD_DEPRECATED STREQUAL board_cli_argument)))
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
message(STATUS "Board: ${BOARD}")

if(DEFINED ENV{ZEPHYR_BOARD_ALIASES})
  include($ENV{ZEPHYR_BOARD_ALIASES})
  if(${BOARD}_BOARD_ALIAS)
    set(BOARD_ALIAS ${BOARD} CACHE STRING "Board alias, provided by user")
    set(BOARD ${${BOARD}_BOARD_ALIAS})
    message(STATUS "Aliased BOARD=${BOARD_ALIAS} changed to ${BOARD}")
  endif()
endif()
include(${ZEPHYR_BASE}/boards/deprecated.cmake)
if(${BOARD}_DEPRECATED)
  set(BOARD_DEPRECATED ${BOARD} CACHE STRING "Deprecated board name, provided by user")
  set(BOARD ${${BOARD}_DEPRECATED})
  message(WARNING "Deprecated BOARD=${BOARD_DEPRECATED} name specified, board automatically changed to: ${BOARD}.")
endif()

# Store the selected board in the cache
set(CACHED_BOARD ${BOARD} CACHE STRING "Selected board")

# The SHIELD can be set by 3 sources. Through environment variables,
# through the cmake CLI, and through CMakeLists.txt.
#
# CLI has the highest precedence, then comes environment variables,
# and then finally CMakeLists.txt.
#
# A user can ignore all the precedence rules if he simply always uses
# the same source. E.g. always specifies -DSHIELD= on the command line,
# always has an environment variable set, or always has a set(SHIELD
# foo) line in his CMakeLists.txt and avoids mixing sources.
#
# The selected SHIELD can be accessed through the variable 'SHIELD'.

# Read out the cached shield value if present
get_property(cached_shield_value CACHE SHIELD PROPERTY VALUE)

# There are actually 4 sources, the three user input sources, and the
# previously used value (CACHED_SHIELD). The previously used value has
# precedence, and if we detect that the user is trying to change the
# value we give him a warning about needing to clean the build
# directory to be able to change shields.

set(shield_cli_argument ${cached_shield_value}) # Either new or old
if(shield_cli_argument STREQUAL CACHED_SHIELD)
  # We already have a CACHED_SHIELD so there is no new input on the CLI
  unset(shield_cli_argument)
endif()

set(shield_app_cmake_lists ${SHIELD})
if(cached_shield_value STREQUAL SHIELD)
  # The app build scripts did not set a default, The SHIELD we are
  # reading is the cached value from the CLI
  unset(shield_app_cmake_lists)
endif()

if(CACHED_SHIELD)
  # Warn the user if it looks like he is trying to change the shield
  # without cleaning first
  if(shield_cli_argument)
    if(NOT (CACHED_SHIELD STREQUAL shield_cli_argument))
      message(WARNING "The build directory must be cleaned pristinely when changing shields")
      # TODO: Support changing shields without requiring a clean build
    endif()
  endif()

  set(SHIELD ${CACHED_SHIELD})
elseif(shield_cli_argument)
  set(SHIELD ${shield_cli_argument})

elseif(DEFINED ENV{SHIELD})
  set(SHIELD $ENV{SHIELD})

elseif(shield_app_cmake_lists)
  set(SHIELD ${shield_app_cmake_lists})
endif()

# Store the selected shield in the cache
set(CACHED_SHIELD ${SHIELD} CACHE STRING "Selected shield")

# 'BOARD_ROOT' is a prioritized list of directories where boards may
# be found. It always includes ${ZEPHYR_BASE} at the lowest priority.
list(APPEND BOARD_ROOT ${ZEPHYR_BASE})

# 'SOC_ROOT' is a prioritized list of directories where socs may be
# found. It always includes ${ZEPHYR_BASE}/soc at the lowest priority.
list(APPEND SOC_ROOT ${ZEPHYR_BASE})

# 'ARCH_ROOT' is a prioritized list of directories where archs may be
# found. It always includes ${ZEPHYR_BASE} at the lowest priority.
list(APPEND ARCH_ROOT ${ZEPHYR_BASE})

if(DEFINED SHIELD)
  string(REPLACE " " ";" SHIELD_AS_LIST "${SHIELD}")
endif()
# SHIELD-NOTFOUND is a real CMake list, from which valid shields can be popped.
# After processing all shields, only invalid shields will be left in this list.
set(SHIELD-NOTFOUND ${SHIELD_AS_LIST})

# Use BOARD to search for a '_defconfig' file.
# e.g. zephyr/boards/arm/96b_carbon_nrf51/96b_carbon_nrf51_defconfig.
# When found, use that path to infer the ARCH we are building for.
foreach(root ${BOARD_ROOT})
  # NB: find_path will return immediately if the output variable is
  # already set
  if (BOARD_ALIAS)
    find_path(BOARD_HIDDEN_DIR
      NAMES ${BOARD_ALIAS}_defconfig
      PATHS ${root}/boards/*/*
      NO_DEFAULT_PATH
      )
    if(BOARD_HIDDEN_DIR)
      message("Board alias ${BOARD_ALIAS} is hiding the real board of same name")
    endif()
  endif()
  find_path(BOARD_DIR
    NAMES ${BOARD}_defconfig
    PATHS ${root}/boards/*/*
    NO_DEFAULT_PATH
    )
  if(BOARD_DIR AND NOT (${root} STREQUAL ${ZEPHYR_BASE}))
    set(USING_OUT_OF_TREE_BOARD 1)
  endif()

  set(shield_dir ${root}/boards/shields)
  # Match the .overlay files in the shield directories to make sure we are
  # finding shields, e.g. x_nucleo_iks01a1/x_nucleo_iks01a1.overlay
  file(GLOB_RECURSE shields_refs_list
    RELATIVE ${shield_dir}
    ${shield_dir}/*/*.overlay
    )

  # The above gives a list like
  # x_nucleo_iks01a1/x_nucleo_iks01a1.overlay;x_nucleo_iks01a2/x_nucleo_iks01a2.overlay
  # we construct a list of shield names by extracting file name and
  # removing the extension.
  unset(SHIELD_LIST)
  foreach(shield_path ${shields_refs_list})
    get_filename_component(shield ${shield_path} NAME_WE)
    list(APPEND SHIELD_LIST ${shield})
  endforeach()

  if(DEFINED SHIELD)
    foreach(s ${SHIELD_AS_LIST})
      list(FIND SHIELD_LIST ${s} _idx)
      if (_idx EQUAL -1)
        continue()
      endif()

      list(REMOVE_ITEM SHIELD-NOTFOUND ${s})

      list(GET shields_refs_list ${_idx} s_path)
      get_filename_component(s_dir ${s_path} DIRECTORY)

      # if shield config flag is on, add shield overlay to the shield overlays
      # list and dts_fixup file to the shield fixup file
      list(APPEND
        shield_dts_files
        ${shield_dir}/${s_path}
        )
      list(APPEND
        shield_dts_fixups
        ${shield_dir}/${s_dir}/dts_fixup.h
        )

      # search for shield/boards/board.overlay file
      if(EXISTS ${shield_dir}/${s_dir}/boards/${BOARD}.overlay)
        # add shield/board overlay to the shield overlays list
        list(APPEND
          shield_dts_files
          ${shield_dir}/${s_dir}/boards/${BOARD}.overlay
          )
      endif()

      # search for shield/boards/shield/board.overlay file
      if(EXISTS ${shield_dir}/${s_dir}/boards/${s}/${BOARD}.overlay)
        # add shield/board overlay to the shield overlays list
        list(APPEND
          shield_dts_files
          ${shield_dir}/${s_dir}/boards/${s}/${BOARD}.overlay
          )
      endif()

      # search for shield/shield.conf file
      if(EXISTS ${shield_dir}/${s_dir}/${s}.conf)
        # add shield.conf to the shield config list
        list(APPEND
          shield_conf_files
          ${shield_dir}/${s_dir}/${s}.conf
          )
      endif()

      # search for shield/boards/board.conf file
      if(EXISTS ${shield_dir}/${s_dir}/boards/${BOARD}.conf)
        # add HW specific board.conf to the shield config list
        list(APPEND
          shield_conf_files
          ${shield_dir}/${s_dir}/boards/${BOARD}.conf
          )
      endif()

      # search for shield/boards/shield/board.conf file
      if(EXISTS ${shield_dir}/${s_dir}/boards/${s}/${BOARD}.conf)
        # add HW specific board.conf to the shield config list
        list(APPEND
          shield_conf_files
          ${shield_dir}/${s_dir}/boards/${s}/${BOARD}.conf
          )
      endif()
    endforeach()
  endif()
endforeach()

if(NOT BOARD_DIR)
  message("No board named '${BOARD}' found")
  print_usage()
  unset(CACHED_BOARD CACHE)
  message(FATAL_ERROR "Invalid usage")
endif()

if(DEFINED SHIELD AND NOT (SHIELD-NOTFOUND STREQUAL ""))
  foreach (s ${SHIELD-NOTFOUND})
    message("No shield named '${s}' found")
  endforeach()
  print_usage()
  unset(CACHED_SHIELD CACHE)
  message(FATAL_ERROR "Invalid usage")
endif()

get_filename_component(BOARD_ARCH_DIR ${BOARD_DIR}      DIRECTORY)
get_filename_component(BOARD_FAMILY   ${BOARD_DIR}      NAME)
get_filename_component(ARCH           ${BOARD_ARCH_DIR} NAME)

foreach(root ${ARCH_ROOT})
  if(EXISTS ${root}/arch/${ARCH}/CMakeLists.txt)
    set(ARCH_DIR ${root}/arch)
    break()
  endif()
endforeach()

if(NOT ARCH_DIR)
  message(FATAL_ERROR "Could not find ARCH=${ARCH} for BOARD=${BOARD}, \
please check your installation. ARCH roots searched: \n\
${ARCH_ROOT}")
endif()

if(CONF_FILE)
  # CONF_FILE has either been specified on the cmake CLI or is already
  # in the CMakeCache.txt. This has precedence over the environment
  # variable CONF_FILE and the default prj.conf
elseif(DEFINED ENV{CONF_FILE})
  set(CONF_FILE $ENV{CONF_FILE})

elseif(COMMAND set_conf_file)
  message(WARNING "'set_conf_file' is deprecated, it will be removed in a future release.")
  set_conf_file()

elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)

elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj.conf ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)

elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/prj.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj.conf)
endif()

set(CONF_FILE ${CONF_FILE} CACHE STRING "If desired, you can build the application using\
the configuration settings specified in an alternate .conf file using this parameter. \
These settings will override the settings in the application’s .config file or its default .conf file.\
Multiple files may be listed, e.g. CONF_FILE=\"prj1.conf prj2.conf\"")

if(ZEPHYR_EXTRA_MODULES)
  # ZEPHYR_EXTRA_MODULES has either been specified on the cmake CLI or is
  # already in the CMakeCache.txt. This has precedence over the environment
  # variable ZEPHYR_EXTRA_MODULES
elseif(DEFINED ENV{ZEPHYR_EXTRA_MODULES})
  set(ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_EXTRA_MODULES})
endif()

if(DTC_OVERLAY_FILE)
  # DTC_OVERLAY_FILE has either been specified on the cmake CLI or is already
  # in the CMakeCache.txt. This has precedence over the environment
  # variable DTC_OVERLAY_FILE
elseif(DEFINED ENV{DTC_OVERLAY_FILE})
  set(DTC_OVERLAY_FILE $ENV{DTC_OVERLAY_FILE})
elseif(EXISTS          ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.overlay)
  set(DTC_OVERLAY_FILE ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.overlay)
elseif(EXISTS          ${APPLICATION_SOURCE_DIR}/${BOARD}.overlay)
  set(DTC_OVERLAY_FILE ${APPLICATION_SOURCE_DIR}/${BOARD}.overlay)
elseif(EXISTS          ${APPLICATION_SOURCE_DIR}/app.overlay)
  set(DTC_OVERLAY_FILE ${APPLICATION_SOURCE_DIR}/app.overlay)
endif()

set(DTC_OVERLAY_FILE ${DTC_OVERLAY_FILE} CACHE STRING "If desired, you can \
build the application using the DT configuration settings specified in an \
alternate .overlay file using this parameter. These settings will override the \
settings in the board's .dts file. Multiple files may be listed, e.g. \
DTC_OVERLAY_FILE=\"dts1.overlay dts2.overlay\"")

# Prevent CMake from testing the toolchain
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

include(${ZEPHYR_BASE}/cmake/host-tools.cmake)

# Include board specific device-tree flags before parsing.
include(${BOARD_DIR}/pre_dt_board.cmake OPTIONAL)

# DTS should be close to kconfig because CONFIG_ variables from
# kconfig and dts should be available at the same time.
#
# The DT system uses a C preprocessor for it's code generation needs.
# This creates an awkward chicken-and-egg problem, because we don't
# always know exactly which toolchain the user needs until we know
# more about the target, e.g. after DT and Kconfig.
#
# To resolve this we find "some" C toolchain, configure it generically
# with the minimal amount of configuration needed to have it
# preprocess DT sources, and then, after we have finished processing
# both DT and Kconfig we complete the target-specific configuration,
# and possibly change the toolchain.
include(${ZEPHYR_BASE}/cmake/generic_toolchain.cmake)
include(${ZEPHYR_BASE}/cmake/dts.cmake)
include(${ZEPHYR_BASE}/cmake/kconfig.cmake)

set(SOC_NAME   ${CONFIG_SOC})
set(SOC_SERIES ${CONFIG_SOC_SERIES})
set(SOC_FAMILY ${CONFIG_SOC_FAMILY})

if("${SOC_SERIES}" STREQUAL "")
  set(SOC_PATH ${SOC_NAME})
else()
  set(SOC_PATH ${SOC_FAMILY}/${SOC_SERIES})
endif()

# Use SOC to search for a 'CMakeLists.txt' file.
# e.g. zephyr/soc/xtense/intel_apl_adsp/CMakeLists.txt.
foreach(root ${SOC_ROOT})
  if(EXISTS ${root}/soc/${ARCH}/${SOC_PATH})
    set(SOC_DIR ${root}/soc)
    break()
  endif()
endforeach()

if(NOT SOC_DIR)
  message(FATAL_ERROR "Could not find SOC=${SOC_NAME} for BOARD=${BOARD}, \
please check your installation. SOC roots searched: \n\
${SOC_ROOT}")
endif()

include(${ZEPHYR_BASE}/cmake/target_toolchain.cmake)

project(Zephyr-Kernel VERSION ${PROJECT_VERSION})
enable_language(C CXX ASM)

# 'project' sets PROJECT_BINARY_DIR to ${CMAKE_CURRENT_BINARY_DIR},
# but for legacy reasons we need it to be set to
# ${CMAKE_CURRENT_BINARY_DIR}/zephyr
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/zephyr)
set(PROJECT_SOURCE_DIR ${ZEPHYR_BASE})

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

# If we are using a suitable ethernet driver inside qemu, then these options
# must be set, otherwise a zephyr instance cannot receive any network packets.
# The Qemu supported ethernet driver should define CONFIG_ETH_NIC_MODEL
# string that tells what nic model Qemu should use.
if(CONFIG_QEMU_TARGET)
  if ((CONFIG_NET_QEMU_ETHERNET OR CONFIG_NET_QEMU_USER) AND NOT CONFIG_ETH_NIC_MODEL)
    message(FATAL_ERROR "
      No Qemu ethernet driver configured!
      Enable Qemu supported ethernet driver like e1000 at drivers/ethernet"
    )
  elseif(CONFIG_NET_QEMU_ETHERNET)
    if(CONFIG_ETH_QEMU_EXTRA_ARGS)
      set(NET_QEMU_ETH_EXTRA_ARGS ",${CONFIG_ETH_QEMU_EXTRA_ARGS}")
    endif()
    list(APPEND QEMU_FLAGS_${ARCH}
      -nic tap,model=${CONFIG_ETH_NIC_MODEL},script=no,downscript=no,ifname=${CONFIG_ETH_QEMU_IFACE_NAME}${NET_QEMU_ETH_EXTRA_ARGS}
    )
  elseif(CONFIG_NET_QEMU_USER)
    list(APPEND QEMU_FLAGS_${ARCH}
      -nic user,model=${CONFIG_ETH_NIC_MODEL},${CONFIG_NET_QEMU_USER_EXTRA_ARGS}
    )
  else()
    list(APPEND QEMU_FLAGS_${ARCH}
      -net none
    )
  endif()
endif()

# General purpose Zephyr target.
# This target can be used for custom zephyr settings that needs to be used elsewhere in the build system
#
# Currently used properties:
# - COMPILES_OPTIONS: Used by application memory partition feature
add_custom_target(zephyr_property_target)

# "app" is a CMake library containing all the application code and is
# modified by the entry point ${APPLICATION_SOURCE_DIR}/CMakeLists.txt
# that was specified when cmake was called.
zephyr_library_named(app)
set_property(TARGET app PROPERTY ARCHIVE_OUTPUT_DIRECTORY app)

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

if("${CMAKE_EXTRA_GENERATOR}" STREQUAL "Eclipse CDT4")
  # Call the amendment function before .project and .cproject generation
  # C and CXX includes, defines in .cproject without __cplusplus
  # with project includes and defines
  include(${ZEPHYR_BASE}/cmake/ide/eclipse_cdt4_generator_amendment.cmake)
  eclipse_cdt4_generator_amendment(1)
endif()
