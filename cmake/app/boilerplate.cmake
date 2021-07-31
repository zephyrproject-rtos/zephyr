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

# Use the old CMake behaviour until we are updating the CMake 3.20 as minimum
# required. This ensure that CMake >=3.20 will be consistent with older CMakes.
# CMP0116: Ninja generators transform DEPFILE s from add_custom_command().
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.20)
  cmake_policy(SET CMP0116 OLD)
endif()

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

if(${CMAKE_VERSION} VERSION_EQUAL 3.19.0 OR
   ${CMAKE_VERSION} VERSION_EQUAL 3.19.1)
  message(WARNING "CMake 3.19.0/3.19.1 contains a bug regarding Toolchain/compiler "
          "testing. Consider switching to a different CMake version.\n"
          "See more here: \n"
          "- https://github.com/zephyrproject-rtos/zephyr/issues/30232\n"
          "- https://gitlab.kitware.com/cmake/cmake/-/issues/21497")
  # This is a workaround for #30232.
  # During Zephyr CMake invocation a plain C compiler is used for DTS.
  # This results in the internal `CheckCompilerFlag.cmake` being included by CMake
  # Later, when the full toolchain is configured, then `CMakeCheckCompilerFlag.cmake` is included.
  # This overloads the `cmake_check_compiler_flag()` function, thus causing #30232.
  # By manualy loading `CMakeCheckCompilerFlag.cmake` then `CheckCompilerFlag.cmake` will overload
  # the functions (and thus win the battle), and because `include_guard(GLOBAL)` is used in
  # `CMakeCheckCompilerFlag.cmake` this file will not be re-included later.
  include(${CMAKE_ROOT}/Modules/Internal/CMakeCheckCompilerFlag.cmake)
endif()

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

set(AUTOCONF_H ${__build_dir}/include/generated/autoconf.h)
# Re-configure (Re-execute all CMakeLists.txt code) when autoconf.h changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})


#
# Import more CMake functions and macros
#

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(${ZEPHYR_BASE}/cmake/extensions.cmake)
include(${ZEPHYR_BASE}/cmake/git.cmake)
include(${ZEPHYR_BASE}/cmake/version.cmake)  # depends on hex.cmake

#
# Find tools
#

include(${ZEPHYR_BASE}/cmake/python.cmake)
include(${ZEPHYR_BASE}/cmake/west.cmake)
include(${ZEPHYR_BASE}/cmake/ccache.cmake)

if(ZEPHYR_EXTRA_MODULES)
  # ZEPHYR_EXTRA_MODULES has either been specified on the cmake CLI or is
  # already in the CMakeCache.txt. This has precedence over the environment
  # variable ZEPHYR_EXTRA_MODULES
elseif(DEFINED ENV{ZEPHYR_EXTRA_MODULES})
  set(ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_EXTRA_MODULES})
endif()

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
  COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${APPLICATION_BINARY_DIR}
          -DSOURCE_DIR=${APPLICATION_SOURCE_DIR}
          -P ${ZEPHYR_BASE}/cmake/pristine.cmake
  # Equivalent to rm -rf build/*
  )

# Dummy add to generate files.
zephyr_linker_sources(SECTIONS)

# 'BOARD_ROOT' is a prioritized list of directories where boards may
# be found. It always includes ${ZEPHYR_BASE} at the lowest priority.
zephyr_file(APPLICATION_ROOT BOARD_ROOT)
list(APPEND BOARD_ROOT ${ZEPHYR_BASE})

# 'SOC_ROOT' is a prioritized list of directories where socs may be
# found. It always includes ${ZEPHYR_BASE}/soc at the lowest priority.
zephyr_file(APPLICATION_ROOT SOC_ROOT)
list(APPEND SOC_ROOT ${ZEPHYR_BASE})

# 'ARCH_ROOT' is a prioritized list of directories where archs may be
# found. It always includes ${ZEPHYR_BASE} at the lowest priority.
zephyr_file(APPLICATION_ROOT ARCH_ROOT)
list(APPEND ARCH_ROOT ${ZEPHYR_BASE})

# Check that BOARD has been provided, and that it has not changed.
zephyr_check_cache(BOARD REQUIRED)

string(FIND "${BOARD}" "@" REVISION_SEPARATOR_INDEX)
if(NOT (REVISION_SEPARATOR_INDEX EQUAL -1))
  math(EXPR BOARD_REVISION_INDEX "${REVISION_SEPARATOR_INDEX} + 1")
  string(SUBSTRING ${BOARD} ${BOARD_REVISION_INDEX} -1 BOARD_REVISION)
  string(SUBSTRING ${BOARD} 0 ${REVISION_SEPARATOR_INDEX} BOARD)
endif()

set(BOARD_MESSAGE "Board: ${BOARD}")

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

zephyr_boilerplate_watch(BOARD)

foreach(root ${BOARD_ROOT})
  # Check that the board root looks reasonable.
  if(NOT IS_DIRECTORY "${root}/boards")
    message(WARNING "BOARD_ROOT element without a 'boards' subdirectory:
${root}
Hints:
  - if your board directory is '/foo/bar/boards/<ARCH>/my_board' then add '/foo/bar' to BOARD_ROOT, not the entire board directory
  - if in doubt, use absolute paths")
  endif()

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
endforeach()

if(EXISTS ${BOARD_DIR}/revision.cmake)
  # Board provides revision handling.
  include(${BOARD_DIR}/revision.cmake)
elseif(BOARD_REVISION)
  message(WARNING "Board revision ${BOARD_REVISION} specified for ${BOARD}, \
                   but board has no revision so revision will be ignored.")
endif()

if(DEFINED BOARD_REVISION)
  set(BOARD_MESSAGE "${BOARD_MESSAGE}, Revision: ${BOARD_REVISION}")
  if(DEFINED ACTIVE_BOARD_REVISION)
    set(BOARD_MESSAGE "${BOARD_MESSAGE} (Active: ${ACTIVE_BOARD_REVISION})")
    set(BOARD_REVISION ${ACTIVE_BOARD_REVISION})
  endif()

  string(REPLACE "." "_" BOARD_REVISION_STRING ${BOARD_REVISION})
endif()

# Check that SHIELD has not changed.
zephyr_check_cache(SHIELD WATCH)

if(SHIELD)
  set(BOARD_MESSAGE "${BOARD_MESSAGE}, Shield(s): ${SHIELD}")
endif()

message(STATUS "${BOARD_MESSAGE}")

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
  set(shield_dir ${root}/boards/shields)
  # Match the Kconfig.shield files in the shield directories to make sure we are
  # finding shields, e.g. x_nucleo_iks01a1/Kconfig.shield
  file(GLOB_RECURSE shields_refs_list ${shield_dir}/*/Kconfig.shield)

  # The above gives a list like
  # x_nucleo_iks01a1/Kconfig.shield;x_nucleo_iks01a2/Kconfig.shield
  # we construct a list of shield names by extracting the folder and find
  # and overlay files in there. Each overlay corresponds to a shield.
  # We obtain the shield name by removing the overlay extension.
  unset(SHIELD_LIST)
  foreach(shields_refs ${shields_refs_list})
    get_filename_component(shield_path ${shields_refs} DIRECTORY)
    file(GLOB shield_overlays RELATIVE ${shield_path} ${shield_path}/*.overlay)
    foreach(overlay ${shield_overlays})
      get_filename_component(shield ${overlay} NAME_WE)
      list(APPEND SHIELD_LIST ${shield})
      set(SHIELD_DIR_${shield} ${shield_path})
    endforeach()
  endforeach()

  if(DEFINED SHIELD)
    foreach(s ${SHIELD_AS_LIST})
      if(NOT ${s} IN_LIST SHIELD_LIST)
        continue()
      endif()

      list(REMOVE_ITEM SHIELD-NOTFOUND ${s})

      # if shield config flag is on, add shield overlay to the shield overlays
      # list and dts_fixup file to the shield fixup file
      list(APPEND
        shield_dts_files
        ${SHIELD_DIR_${s}}/${s}.overlay
        )

      list(APPEND
        shield_dts_fixups
        ${SHIELD_DIR_${s}}/dts_fixup.h
        )

      list(APPEND
        SHIELD_DIRS
        ${SHIELD_DIR_${s}}
        )

      # search for shield/shield.conf file
      if(EXISTS ${SHIELD_DIR_${s}}/${s}.conf)
        # add shield.conf to the shield config list
        list(APPEND
          shield_conf_files
          ${SHIELD_DIR_${s}}/${s}.conf
          )
      endif()

      zephyr_file(CONF_FILES ${SHIELD_DIR_${s}}/boards
                  DTS   shield_dts_files
                  KCONF shield_conf_files
      )
      zephyr_file(CONF_FILES ${SHIELD_DIR_${s}}/boards/${s}
                  DTS   shield_dts_files
                  KCONF shield_conf_files
      )
    endforeach()
  endif()
endforeach()

if(NOT BOARD_DIR)
  message("No board named '${BOARD}' found.

Please choose one of the following boards:
")
  execute_process(
    COMMAND
    ${CMAKE_COMMAND}
    -DZEPHYR_BASE=${ZEPHYR_BASE}
    -DBOARD_ROOT=${BOARD_ROOT}
    -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
    -P ${ZEPHYR_BASE}/cmake/boards.cmake
    )
  unset(CACHED_BOARD CACHE)
  message(FATAL_ERROR "Invalid BOARD; see above.")
endif()

if(DEFINED SHIELD AND NOT (SHIELD-NOTFOUND STREQUAL ""))
  foreach (s ${SHIELD-NOTFOUND})
    message("No shield named '${s}' found")
  endforeach()
  message("Please choose from among the following shields:")
  string(REPLACE ";" "\\;" SHIELD_LIST_ESCAPED "${SHIELD_LIST}")
  execute_process(
    COMMAND
    ${CMAKE_COMMAND}
    -DZEPHYR_BASE=${ZEPHYR_BASE}
    -DSHIELD_LIST=${SHIELD_LIST_ESCAPED}
    -P ${ZEPHYR_BASE}/cmake/shields.cmake
    )
  unset(CACHED_SHIELD CACHE)
  message(FATAL_ERROR "Invalid SHIELD; see above.")
endif()

get_filename_component(BOARD_ARCH_DIR ${BOARD_DIR}      DIRECTORY)
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
    get_filename_component(CONF_FILE_DIR ${CONF_FILE} DIRECTORY)
    if(${CONF_FILE_NAME} MATCHES "prj_(.*).conf")
      set(CONF_FILE_BUILD_TYPE ${CMAKE_MATCH_1})
      set(CONF_FILE_INCLUDE_FRAGMENTS true)

      if(NOT IS_ABSOLUTE ${CONF_FILE_DIR})
        set(CONF_FILE_DIR ${APPLICATION_SOURCE_DIR}/${CONF_FILE_DIR})
      endif()
    endif()
  endif()
elseif(CACHED_CONF_FILE)
  # Cached conf file is present.
  # That value has precedence over anything else than a new
  # `cmake -DCONF_FILE=<file>` invocation.
  set(CONF_FILE ${CACHED_CONF_FILE})
elseif(DEFINED ENV{CONF_FILE})
  set(CONF_FILE $ENV{CONF_FILE})

elseif(COMMAND set_conf_file)
  message(WARNING "'set_conf_file' is deprecated, it will be removed in a future release.")
  set_conf_file()

elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)

elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/prj.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj.conf)
  set(CONF_FILE_INCLUDE_FRAGMENTS true)
endif()

if(CONF_FILE_INCLUDE_FRAGMENTS)
  if(NOT CONF_FILE_DIR)
     set(CONF_FILE_DIR ${APPLICATION_SOURCE_DIR})
  endif()
  zephyr_file(CONF_FILES ${CONF_FILE_DIR}/boards KCONF CONF_FILE BUILD ${CONF_FILE_BUILD_TYPE})
endif()

set(CACHED_CONF_FILE ${CONF_FILE} CACHE STRING "If desired, you can build the application using\
the configuration settings specified in an alternate .conf file using this parameter. \
These settings will override the settings in the application’s .config file or its default .conf file.\
Multiple files may be listed, e.g. CONF_FILE=\"prj1.confi;prj2.conf\" \
The CACHED_CONF_FILE is internal Zephyr variable used between CMake runs. \
To change CONF_FILE, use the CONF_FILE variable.")
unset(CONF_FILE CACHE)

zephyr_file(CONF_FILES ${APPLICATION_SOURCE_DIR}/boards DTS APP_BOARD_DTS)

# The CONF_FILE variable is now set to its final value.
zephyr_boilerplate_watch(CONF_FILE)

if(DTC_OVERLAY_FILE)
  # DTC_OVERLAY_FILE has either been specified on the cmake CLI or is already
  # in the CMakeCache.txt. This has precedence over the environment
  # variable DTC_OVERLAY_FILE
elseif(DEFINED ENV{DTC_OVERLAY_FILE})
  set(DTC_OVERLAY_FILE $ENV{DTC_OVERLAY_FILE})
elseif(APP_BOARD_DTS)
  set(DTC_OVERLAY_FILE ${APP_BOARD_DTS})
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

# Populate USER_CACHE_DIR with a directory that user applications may
# write cache files to.
if(NOT DEFINED USER_CACHE_DIR)
  find_appropriate_cache_directory(USER_CACHE_DIR)
endif()
message(STATUS "Cache files will be written to: ${USER_CACHE_DIR}")

# Prevent CMake from testing the toolchain
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

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

set(SOC_NAME   ${CONFIG_SOC})
set(SOC_SERIES ${CONFIG_SOC_SERIES})
set(SOC_TOOLCHAIN_NAME ${CONFIG_SOC_TOOLCHAIN_NAME})
set(SOC_FAMILY ${CONFIG_SOC_FAMILY})

if("${SOC_SERIES}" STREQUAL "")
  set(SOC_PATH ${SOC_NAME})
else()
  set(SOC_PATH ${SOC_FAMILY}/${SOC_SERIES})
endif()

# Use SOC to search for a 'CMakeLists.txt' file.
# e.g. zephyr/soc/xtense/intel_apl_adsp/CMakeLists.txt.
foreach(root ${SOC_ROOT})
  # Check that the root looks reasonable.
  if(NOT IS_DIRECTORY "${root}/soc")
    message(WARNING "SOC_ROOT element without a 'soc' subdirectory:
${root}
Hints:
  - if your SoC family directory is '/foo/bar/soc/<ARCH>/my_soc_family', then add '/foo/bar' to SOC_ROOT, not the entire SoC family path
  - if in doubt, use absolute paths")
  endif()

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
