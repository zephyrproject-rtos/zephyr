# Copyright (c) 2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# This script generates a tarball containing all headers and flags necessary to
# build an llext extension. It does so by copying all headers accessible from
# INTERFACE_INCLUDE_DIRECTORIES and generating a Makefile.cflags file (and a
# cmake.cflags one) with all flags necessary to build the extension.
#
# The tarball can be extracted and used in the extension build system to include
# all necessary headers and flags. File paths are made relative to a few key
# directories (build/zephyr, zephyr base, west top dir and application source
# dir), to avoid leaking any information about the host system.
#
# The following arguments are expected:
# - llext_edk_name: Name of the extension, used to name the tarball and the
#   install directory variable for Makefile.
# - INTERFACE_INCLUDE_DIRECTORIES: List of include directories to copy headers
#   from. It should simply be the INTERFACE_INCLUDE_DIRECTORIES property of the
#   zephyr_interface target.
# - llext_edk_file: Output file name for the tarball.
# - llext_edk_cflags: Flags to be used for source compile commands.
# - ZEPHYR_BASE: Path to the zephyr base directory.
# - WEST_TOPDIR: Path to the west top directory.
# - APPLICATION_SOURCE_DIR: Path to the application source directory.
# - PROJECT_BINARY_DIR: Path to the project binary build directory.
# - CONFIG_LLEXT_EDK_USERSPACE_ONLY: Whether to copy syscall headers from the
#   edk directory. This is necessary when building an extension that only
#   supports userspace, as the syscall headers are regenerated in the edk
#   directory.

cmake_minimum_required(VERSION 3.20.0)

# Usage:
#   relative_dir(<dir> <relative_out> <bindir_out>)
#
# Helper function to generate relative paths to a few key directories
# (PROJECT_BINARY_DIR, ZEPHYR_BASE, WEST_TOPDIR and APPLICATION_SOURCE_DIR).
# The generated path is relative to the key directory, and the bindir_out
# output variable is set to TRUE if the path is relative to PROJECT_BINARY_DIR.
#
function(relative_dir dir relative_out bindir_out)
    cmake_path(IS_PREFIX PROJECT_BINARY_DIR ${dir} NORMALIZE to_prj_bindir)
    cmake_path(IS_PREFIX ZEPHYR_BASE ${dir} NORMALIZE to_zephyr_base)
    if("${WEST_TOPDIR}" STREQUAL "")
        set(to_west_topdir FALSE)
    else()
        cmake_path(IS_PREFIX WEST_TOPDIR ${dir} NORMALIZE to_west_topdir)
    endif()
    cmake_path(IS_PREFIX APPLICATION_SOURCE_DIR ${dir} NORMALIZE to_app_srcdir)

    # Overall idea is to place included files in the destination dir based on the source:
    # files coming from build/zephyr/generated will end up at
    # <install-dir>/include/zephyr/include/generated, files coming from zephyr base at
    # <install-dir>/include/zephyr/include, files from west top dir (for instance, hal modules),
    # at <install-dir>/include and application ones at <install-dir>/include/<application-dir>.
    # Finally, everything else (such as external libs not at any of those places) will end up
    # at <install-dir>/include/<full-path-to-external-include>, so we avoid any external lib
    # stepping at any other lib toes.
    if(to_prj_bindir)
        cmake_path(RELATIVE_PATH dir BASE_DIRECTORY ${PROJECT_BINARY_DIR} OUTPUT_VARIABLE dir_tmp)
        set(dest ${llext_edk_inc}/zephyr/${dir_tmp})
    elseif(to_zephyr_base)
        cmake_path(RELATIVE_PATH dir BASE_DIRECTORY ${ZEPHYR_BASE} OUTPUT_VARIABLE dir_tmp)
        set(dest ${llext_edk_inc}/zephyr/${dir_tmp})
    elseif(to_west_topdir)
        cmake_path(RELATIVE_PATH dir BASE_DIRECTORY ${WEST_TOPDIR} OUTPUT_VARIABLE dir_tmp)
        set(dest ${llext_edk_inc}/${dir_tmp})
    elseif(to_app_srcdir)
        cmake_path(GET APPLICATION_SOURCE_DIR FILENAME app_dir)
        cmake_path(RELATIVE_PATH dir BASE_DIRECTORY ${APPLICATION_SOURCE_DIR} OUTPUT_VARIABLE dir_tmp)
        set(dest ${llext_edk_inc}/${app_dir}/${dir_tmp})
    else()
        set(dest ${llext_edk_inc}/${dir})
    endif()

    set(${relative_out} ${dest} PARENT_SCOPE)
    if(to_prj_bindir)
        set(${bindir_out} TRUE PARENT_SCOPE)
    else()
        set(${bindir_out} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Usage:
#   edk_write_header(<target>)
#
# Initialize the file associated with <target> and write its header.
function(edk_write_header target)
    file(WRITE ${edk_file_${target}} "")
endfunction()

# Usage:
#   edk_write_var(<target> <var_name> <var_value>)
#
# Write to the file associated with <target> an entry where <var_name> is
# assigned the value <var_value>.
function(edk_write_var target var_name var_value)
    if(target STREQUAL "CMAKE")
        # CMake: export assignments of the form:
        #
        #   set(var value1;value2;...)
        #
        file(APPEND ${edk_file_${target}} "set(${var_name} ${var_value})\n")
    elseif(target STREQUAL "MAKEFILE")
        # Makefile: export assignments of the form:
        #
        #   var = value1 value2 ...
        #
        list(JOIN var_value " " var_value_str)
        file(APPEND ${edk_file_${target}} "${var_name} = ${var_value_str}\n")
    endif()
endfunction()



if (CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID)
  message(FATAL_ERROR
    "The LLEXT EDK is not compatible with CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID.")
endif()

set(llext_edk ${PROJECT_BINARY_DIR}/${llext_edk_name})
set(llext_edk_inc ${llext_edk}/include)

string(REGEX REPLACE "[^a-zA-Z0-9]" "_" llext_edk_name_sane ${llext_edk_name})
string(TOUPPER ${llext_edk_name_sane} llext_edk_name_sane)
set(install_dir_var "${llext_edk_name_sane}_INSTALL_DIR")

separate_arguments(llext_edk_cflags NATIVE_COMMAND ${llext_edk_cflags})

set(make_relative FALSE)
foreach(flag ${llext_edk_cflags})
    if (flag STREQUAL "-imacros")
        set(make_relative TRUE)
    elseif (make_relative)
        set(make_relative FALSE)
        cmake_path(GET flag PARENT_PATH parent)
        cmake_path(GET flag FILENAME name)
        relative_dir(${parent} dest bindir)
        cmake_path(RELATIVE_PATH dest BASE_DIRECTORY ${llext_edk} OUTPUT_VARIABLE dest_rel)
        if(bindir)
            list(APPEND imacros_gen_make "-imacros\$(${install_dir_var})/${dest_rel}/${name}")
            list(APPEND imacros_gen_cmake "-imacros\${CMAKE_CURRENT_LIST_DIR}/${dest_rel}/${name}")
        else()
            list(APPEND imacros_make "-imacros\$(${install_dir_var})/${dest_rel}/${name}")
            list(APPEND imacros_cmake "-imacros\${CMAKE_CURRENT_LIST_DIR}/${dest_rel}/${name}")
        endif()
    else()
        list(APPEND new_cflags ${flag})
    endif()
endforeach()
set(llext_edk_cflags ${new_cflags})

list(APPEND base_flags_make ${llext_edk_cflags} ${imacros_make})
list(APPEND base_flags_cmake ${llext_edk_cflags} ${imacros_cmake})

separate_arguments(include_dirs NATIVE_COMMAND ${INTERFACE_INCLUDE_DIRECTORIES})
file(MAKE_DIRECTORY ${llext_edk_inc})
foreach(dir ${include_dirs})
    if (NOT EXISTS ${dir})
        continue()
    endif()

    relative_dir(${dir} dest bindir)
    # Use destination parent, as the last part of the source directory is copied as well
    cmake_path(GET dest PARENT_PATH dest_p)

    file(MAKE_DIRECTORY ${dest_p})
    file(COPY ${dir} DESTINATION ${dest_p} FILES_MATCHING PATTERN "*.h")

    cmake_path(RELATIVE_PATH dest BASE_DIRECTORY ${llext_edk} OUTPUT_VARIABLE dest_rel)
    if(bindir)
        list(APPEND inc_gen_flags_make "-I\$(${install_dir_var})/${dest_rel}")
        list(APPEND inc_gen_flags_cmake "-I\${CMAKE_CURRENT_LIST_DIR}/${dest_rel}")
    else()
        list(APPEND inc_flags_make "-I\$(${install_dir_var})/${dest_rel}")
        list(APPEND inc_flags_cmake "-I\${CMAKE_CURRENT_LIST_DIR}/${dest_rel}")
    endif()
    list(APPEND all_inc_flags_make "-I\$(${install_dir_var})/${dest_rel}")
    list(APPEND all_inc_flags_cmake "-I\${CMAKE_CURRENT_LIST_DIR}/${dest_rel}")
endforeach()

list(APPEND all_flags_make ${base_flags_make} ${imacros_gen_make} ${all_inc_flags_make})
list(APPEND all_flags_cmake ${base_flags_cmake} ${imacros_gen_cmake} ${all_inc_flags_cmake})

if(CONFIG_LLEXT_EDK_USERSPACE_ONLY)
    # Copy syscall headers from edk directory, as they were regenerated there.
    file(COPY ${PROJECT_BINARY_DIR}/edk/include/generated/ DESTINATION ${llext_edk_inc}/zephyr/include/generated)
endif()


#
# Generate the EDK flags files
#

set(edk_file_MAKEFILE ${llext_edk}/Makefile.cflags)
set(edk_file_CMAKE ${llext_edk}/cmake.cflags)

# Generate flags for Makefile
edk_write_header(MAKEFILE)
edk_write_var(MAKEFILE "LLEXT_CFLAGS" "${all_flags_make}")
edk_write_var(MAKEFILE "LLEXT_ALL_INCLUDE_CFLAGS" "${all_inc_flags_make}")
edk_write_var(MAKEFILE "LLEXT_INCLUDE_CFLAGS" "${inc_flags_make}")
edk_write_var(MAKEFILE "LLEXT_GENERATED_INCLUDE_CFLAGS" "${gen_inc_flags_make}")
edk_write_var(MAKEFILE "LLEXT_BASE_CFLAGS" "${base_flags_make}")
edk_write_var(MAKEFILE "LLEXT_GENERATED_IMACROS_CFLAGS" "${imacros_gen_make}")

# Generate flags for CMake
edk_write_header(CMAKE)
edk_write_var(CMAKE "LLEXT_CFLAGS" "${all_flags_cmake}")
edk_write_var(CMAKE "LLEXT_ALL_INCLUDE_CFLAGS" "${all_inc_flags_cmake}")
edk_write_var(CMAKE "LLEXT_INCLUDE_CFLAGS" "${inc_flags_cmake}")
edk_write_var(CMAKE "LLEXT_GENERATED_INCLUDE_CFLAGS" "${gen_inc_flags_cmake}")
edk_write_var(CMAKE "LLEXT_BASE_CFLAGS" "${base_flags_cmake}")
edk_write_var(CMAKE "LLEXT_GENERATED_IMACROS_CFLAGS" "${imacros_gen_cmake}")

# Generate the tarball
file(ARCHIVE_CREATE
    OUTPUT ${llext_edk_file}
    PATHS ${llext_edk}
    FORMAT gnutar
    COMPRESSION XZ
)

file(REMOVE_RECURSE ${llext_edk})
