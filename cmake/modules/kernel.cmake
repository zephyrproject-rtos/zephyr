# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Zephyr Kernel CMake module.
#
# This is the main Zephyr Kernel CMake module which is responsible for creation
# of Zephyr libraries and the Zephyr executable.
#
# This CMake module creates 'project(Zephyr-Kernel)'
#
# It defines properties to use while configuring libraries to be build as well
# as using add_subdirectory() to add the main <ZEPHYR_BASE>/CMakeLists.txt file.
#
# Outcome:
# - Zephyr build system.
# - Zephyr project
#
# Important libraries:
# - app: This is the main application library where the application can add
#        source files that must be included when building Zephyr

include_guard(GLOBAL)

# As this module is not intended for direct loading, but should be loaded through
# find_package(Zephyr) then it won't be loading any Zephyr CMake modules by itself.

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

# Note any later project() resets PROJECT_SOURCE_DIR
file(TO_CMAKE_PATH "${ZEPHYR_BASE}" PROJECT_SOURCE_DIR)

set(ZEPHYR_BINARY_DIR ${PROJECT_BINARY_DIR})

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

# For the gen_app_partitions.py to work correctly, we must ensure that
# all targets exports their compile commands to fetch object files.
# We enable it unconditionally, as this is also useful for several IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE CACHE BOOL
    "Export CMake compile commands. Used by gen_app_partitions.py script"
    FORCE
)

project(Zephyr-Kernel VERSION ${PROJECT_VERSION})

# Add .S file extension suffix into CMAKE_ASM_SOURCE_FILE_EXTENSIONS,
# because clang from OneApi can't recognize them as asm files on
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
set(KERNEL_SYMBOLS_NAME    ${KERNEL_NAME}.symbols)

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
# NB: This must be done here because 'app' can only be modified in the
# CMakeLists.txt file that created it. And it must be
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

if(ZEPHYR_NRF_MODULE_DIR)
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/partition_manager.cmake)
endif()
