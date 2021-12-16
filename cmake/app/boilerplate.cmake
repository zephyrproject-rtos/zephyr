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

# CMake version 3.20 is the real minimum supported version.
#
# Unfortunately CMake requires the toplevel CMakeLists.txt file to
# define the required version, not even invoking it from an included
# file, like boilerplate.cmake, is sufficient. It is however permitted
# to have multiple invocations of cmake_minimum_required.
#
# Under these restraints we use a second 'cmake_minimum_required'
# invocation in every toplevel CMakeLists.txt.
cmake_minimum_required(VERSION 3.20.0)

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

define_property(GLOBAL PROPERTY GENERATED_APP_SOURCE_FILES
  BRIEF_DOCS "Source files that are generated after Zephyr has been linked once."
  FULL_DOCS "\
Source files that are generated after Zephyr has been linked once.\
May include dev_handles.c etc."
  )
set_property(GLOBAL PROPERTY GENERATED_APP_SOURCE_FILES "")

define_property(GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES
  BRIEF_DOCS "Object files that are generated after symbol addresses are fixed."
  FULL_DOCS "\
Object files that are generated after symbol addresses are fixed.\
May include mmu tables, etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES "")

define_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES
  BRIEF_DOCS "Source files that are generated after symbol addresses are fixed."
  FULL_DOCS "\
Source files that are generated after symbol addresses are fixed.\
May include isr_tables.c etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES "")

set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Application Source Directory")
set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Application Binary Directory")

set(__build_dir ${CMAKE_CURRENT_BINARY_DIR}/zephyr)

set(PROJECT_BINARY_DIR ${__build_dir})

message(STATUS "Application: ${APPLICATION_SOURCE_DIR}")

add_custom_target(code_data_relocation_target)

# The zephyr/runners.yaml file in the build directory is used to
# configure the scripts/west_commands/runners Python package used
# by 'west flash', 'west debug', etc.
#
# This is a helper target for setting property:value pairs related to
# this file:
#
# Property         Description
# --------------   --------------------------------------------------
# bin_file         "zephyr.bin" file for flashing
# hex_file         "zephyr.hex" file for flashing
# elf_file         "zephyr.elf" file for flashing or debugging
# yaml_contents    generated contents of runners.yaml
#
# Note: there are quotes around "zephyr.bin" etc. because the actual
# paths can be changed, e.g. to flash signed versions of these files
# for consumption by bootloaders such as MCUboot.
#
# See cmake/flash/CMakeLists.txt for more details.
add_custom_target(runners_yaml_props_target)

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

find_package(ZephyrBuildConfiguration
  QUIET NO_POLICY_SCOPE
  NAMES ZephyrBuild
  PATHS ${ZEPHYR_BASE}/../*
  NO_CMAKE_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  NO_CMAKE_PACKAGE_REGISTRY
  NO_CMAKE_SYSTEM_PATH
  NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
)

# Note any later project() resets PROJECT_SOURCE_DIR
file(TO_CMAKE_PATH "${ZEPHYR_BASE}" PROJECT_SOURCE_DIR)

set(ZEPHYR_BINARY_DIR ${PROJECT_BINARY_DIR})

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
include(${ZEPHYR_BASE}/cmake/ccache.cmake)

# 'MODULE_EXT_ROOT' is a prioritized list of directories where module glue code
# may be found. It always includes ${ZEPHYR_BASE} at the lowest priority.
# For module roots, later entries may overrule module settings already defined
# by processed module roots, hence first in list means lowest priority.
zephyr_file(APPLICATION_ROOT MODULE_EXT_ROOT)
list(INSERT MODULE_EXT_ROOT 0 ${ZEPHYR_BASE})

#
# Find Zephyr modules.
# Those may contain additional DTS, BOARD, SOC, ARCH ROOTs.
# Also create the Kconfig binary dir for generated Kconf files.
#
include(${ZEPHYR_BASE}/cmake/zephyr_module.cmake)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  message(FATAL_ERROR "Source directory equals build directory.\
 In-source builds are not supported.\
 Please specify a build directory, e.g. cmake -Bbuild -H.")
endif()

add_custom_target(
  pristine
  COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${APPLICATION_BINARY_DIR}
          -DSOURCE_DIR=${APPLICATION_SOURCE_DIR}
          -P ${ZEPHYR_BASE}/cmake/pristine.cmake
  # Equivalent to rm -rf build/*
  )

# Dummy add to generate files.
zephyr_linker_sources(SECTIONS)

zephyr_file(APPLICATION_ROOT BOARD_ROOT)

zephyr_file(APPLICATION_ROOT SOC_ROOT)

zephyr_file(APPLICATION_ROOT ARCH_ROOT)

include(${ZEPHYR_BASE}/cmake/boards.cmake)
include(${ZEPHYR_BASE}/cmake/shields.cmake)
include(${ZEPHYR_BASE}/cmake/arch.cmake)

if(DEFINED APPLICATION_CONFIG_DIR)
  string(CONFIGURE ${APPLICATION_CONFIG_DIR} APPLICATION_CONFIG_DIR)
  if(NOT IS_ABSOLUTE ${APPLICATION_CONFIG_DIR})
    get_filename_component(APPLICATION_CONFIG_DIR ${APPLICATION_CONFIG_DIR} ABSOLUTE)
  endif()
else()
  # Application config dir is not set, so we default to the  application
  # source directory as configuration directory.
  set(APPLICATION_CONFIG_DIR ${APPLICATION_SOURCE_DIR})
endif()

if(DEFINED CONF_FILE)
  # This ensures that CACHE{CONF_FILE} will be set correctly to current scope
  # variable CONF_FILE. An already current scope variable will stay the same.
  set(CONF_FILE ${CONF_FILE})

  # CONF_FILE has either been specified on the cmake CLI or is already
  # in the CMakeCache.txt. This has precedence over the environment
  # variable CONF_FILE and the default prj.conf

  # In order to support a `prj_<name>.conf pattern for auto inclusion of board
  # overlays, then we must first ensure only a single conf file is provided.
  string(REPLACE " " ";" CONF_FILE_AS_LIST "${CONF_FILE}")
  list(LENGTH CONF_FILE_AS_LIST CONF_FILE_LENGTH)
  if(${CONF_FILE_LENGTH} EQUAL 1)
    # Need the file name to look for match.
    # Need path in order to check if it is absolute.
    get_filename_component(CONF_FILE_NAME ${CONF_FILE} NAME)
    if(${CONF_FILE_NAME} MATCHES "prj_(.*).conf")
      set(CONF_FILE_BUILD_TYPE ${CMAKE_MATCH_1})
      set(CONF_FILE_INCLUDE_FRAGMENTS true)
    endif()
  endif()
elseif(CACHED_CONF_FILE)
  # Cached conf file is present.
  # That value has precedence over anything else than a new
  # `cmake -DCONF_FILE=<file>` invocation.
  set(CONF_FILE ${CACHED_CONF_FILE})
elseif(DEFINED ENV{CONF_FILE})
  set(CONF_FILE $ENV{CONF_FILE})

elseif(EXISTS   ${APPLICATION_CONFIG_DIR}/prj_${BOARD}.conf)
  set(CONF_FILE ${APPLICATION_CONFIG_DIR}/prj_${BOARD}.conf)

elseif(EXISTS   ${APPLICATION_CONFIG_DIR}/prj.conf)
  set(CONF_FILE ${APPLICATION_CONFIG_DIR}/prj.conf)
  set(CONF_FILE_INCLUDE_FRAGMENTS true)
endif()

if(CONF_FILE_INCLUDE_FRAGMENTS)
  zephyr_file(CONF_FILES ${APPLICATION_CONFIG_DIR}/boards KCONF CONF_FILE BUILD ${CONF_FILE_BUILD_TYPE})
endif()

set(APPLICATION_CONFIG_DIR ${APPLICATION_CONFIG_DIR} CACHE INTERNAL "The application configuration folder")
set(CACHED_CONF_FILE ${CONF_FILE} CACHE STRING "If desired, you can build the application using\
the configuration settings specified in an alternate .conf file using this parameter. \
These settings will override the settings in the application’s .config file or its default .conf file.\
Multiple files may be listed, e.g. CONF_FILE=\"prj1.confi;prj2.conf\" \
The CACHED_CONF_FILE is internal Zephyr variable used between CMake runs. \
To change CONF_FILE, use the CONF_FILE variable.")
unset(CONF_FILE CACHE)

zephyr_file(CONF_FILES ${APPLICATION_CONFIG_DIR}/boards DTS APP_BOARD_DTS)

# The CONF_FILE variable is now set to its final value.
zephyr_boilerplate_watch(CONF_FILE)

if(DTC_OVERLAY_FILE)
  # DTC_OVERLAY_FILE has either been specified on the cmake CLI or is already
  # in the CMakeCache.txt.
elseif(APP_BOARD_DTS)
  set(DTC_OVERLAY_FILE ${APP_BOARD_DTS})
elseif(EXISTS          ${APPLICATION_CONFIG_DIR}/${BOARD}.overlay)
  set(DTC_OVERLAY_FILE ${APPLICATION_CONFIG_DIR}/${BOARD}.overlay)
elseif(EXISTS          ${APPLICATION_CONFIG_DIR}/app.overlay)
  set(DTC_OVERLAY_FILE ${APPLICATION_CONFIG_DIR}/app.overlay)
endif()

set(DTC_OVERLAY_FILE ${DTC_OVERLAY_FILE} CACHE STRING "If desired, you can \
build the application using the DT configuration settings specified in an \
alternate .overlay file using this parameter. These settings will override the \
settings in the board's .dts file. Multiple files may be listed, e.g. \
DTC_OVERLAY_FILE=\"dts1.overlay dts2.overlay\"")

include(${ZEPHYR_BASE}/cmake/user_cache.cmake)
include(${ZEPHYR_BASE}/cmake/verify-toolchain.cmake)
include(${ZEPHYR_BASE}/cmake/host-tools.cmake)

# Include board specific device-tree flags before parsing.
include(${BOARD_DIR}/pre_dt_board.cmake OPTIONAL)

# The DTC_OVERLAY_FILE variable is now set to its final value.
zephyr_boilerplate_watch(DTC_OVERLAY_FILE)

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
include(${ZEPHYR_BASE}/cmake/soc.cmake)

# For the gen_app_partitions.py to work correctly, we must ensure that
# all targets exports their compile commands to fetch object files.
# We enable it unconditionally, as this is also useful for several IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE CACHE BOOL
    "Export CMake compile commands. Used by gen_app_partitions.py script"
    FORCE
)
include(${ZEPHYR_BASE}/cmake/target_toolchain.cmake)

project(Zephyr-Kernel VERSION ${PROJECT_VERSION})

# Add .S file extension suffix into CMAKE_ASM_SOURCE_FILE_EXTENSIONS,
# because clang from OneApi can't recongnize them as asm files on
# windows now.
list(APPEND CMAKE_ASM_SOURCE_FILE_EXTENSIONS "S")
enable_language(C CXX ASM)

# The setup / configuration of the toolchain itself and the configuration of
# supported compilation flags are now split, as this allows to use the toolchain
# for generic purposes, for example DTS, and then test the toolchain for
# supported flags at stage two.
# Testing the toolchain flags requires the enable_language() to have been called in CMake.

# Verify that the toolchain can compile a dummy file, if it is not we
# won't be able to test for compatibility with certain C flags.
zephyr_check_compiler_flag(C "" toolchain_is_ok)
assert(toolchain_is_ok "The toolchain is unable to build a dummy C file. See CMakeError.log.")

include(${ZEPHYR_BASE}/cmake/target_toolchain_flags.cmake)

# 'project' sets PROJECT_BINARY_DIR to ${CMAKE_CURRENT_BINARY_DIR},
# but for legacy reasons we need it to be set to
# ${CMAKE_CURRENT_BINARY_DIR}/zephyr
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/zephyr)
set(PROJECT_SOURCE_DIR ${ZEPHYR_BASE})

set(KERNEL_NAME ${CONFIG_KERNEL_BIN_NAME})

set(KERNEL_ELF_NAME   ${KERNEL_NAME}.elf)
set(KERNEL_BIN_NAME   ${KERNEL_NAME}.bin)
set(KERNEL_HEX_NAME   ${KERNEL_NAME}.hex)
set(KERNEL_UF2_NAME   ${KERNEL_NAME}.uf2)
set(KERNEL_MAP_NAME   ${KERNEL_NAME}.map)
set(KERNEL_LST_NAME   ${KERNEL_NAME}.lst)
set(KERNEL_S19_NAME   ${KERNEL_NAME}.s19)
set(KERNEL_EXE_NAME   ${KERNEL_NAME}.exe)
set(KERNEL_STAT_NAME  ${KERNEL_NAME}.stat)
set(KERNEL_STRIP_NAME ${KERNEL_NAME}.strip)
set(KERNEL_META_NAME  ${KERNEL_NAME}.meta)

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
