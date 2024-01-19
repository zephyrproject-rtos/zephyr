# Copyright (c) 2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

set(LLEXT_EDK ${PROJECT_BINARY_DIR}/${LLEXT_EDK_NAME})
set(LLEXT_EDK_INC ${LLEXT_EDK}/include)

string(REGEX REPLACE "[^a-zA-Z0-9]" "_" llext_edk_name_sane ${LLEXT_EDK_NAME})
string(TOUPPER ${llext_edk_name_sane} llext_edk_name_sane)
set(install_dir_var "${llext_edk_name_sane}_INSTALL_DIR")

cmake_path(CONVERT "${INTERFACE_INCLUDE_DIRECTORIES}" TO_CMAKE_PATH_LIST INCLUDE_DIRS)

set(autoconf_h_edk ${LLEXT_EDK_INC}/${AUTOCONF_H})
cmake_path(RELATIVE_PATH AUTOCONF_H BASE_DIRECTORY ${PROJECT_BINARY_DIR} OUTPUT_VARIABLE autoconf_h_rel)

list(APPEND all_flags_make
    "${LLEXT_CFLAGS} -imacros\$(${install_dir_var})/include/zephyr/${autoconf_h_rel}")
list(APPEND all_flags_cmake
    "${LLEXT_CFLAGS} -imacros\${${install_dir_var}}/include/zephyr/${autoconf_h_rel}")

file(MAKE_DIRECTORY ${LLEXT_EDK_INC})
foreach(dir ${INCLUDE_DIRS})
    if (NOT EXISTS ${dir})
        continue()
    endif()
    cmake_path(IS_PREFIX PROJECT_BINARY_DIR ${dir} NORMALIZE to_prj_bindir)
    cmake_path(IS_PREFIX ZEPHYR_BASE ${dir} NORMALIZE to_zephyr_base)
    cmake_path(IS_PREFIX WEST_TOPDIR ${dir} NORMALIZE to_west_topdir)
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
        set(dest ${LLEXT_EDK_INC}/zephyr/${dir_tmp})
    elseif(to_zephyr_base)
        cmake_path(RELATIVE_PATH dir BASE_DIRECTORY ${ZEPHYR_BASE} OUTPUT_VARIABLE dir_tmp)
        set(dest ${LLEXT_EDK_INC}/zephyr/${dir_tmp})
    elseif(to_west_topdir)
        cmake_path(RELATIVE_PATH dir BASE_DIRECTORY ${WEST_TOPDIR} OUTPUT_VARIABLE dir_tmp)
        set(dest ${LLEXT_EDK_INC}/${dir_tmp})
    elseif(to_app_srcdir)
        cmake_path(GET APPLICATION_SOURCE_DIR FILENAME app_dir)
        cmake_path(RELATIVE_PATH dir BASE_DIRECTORY ${APPLICATION_SOURCE_DIR} OUTPUT_VARIABLE dir_tmp)
        set(dest ${LLEXT_EDK_INC}/${app_dir}/${dir_tmp})
    else()
        set(dest ${LLEXT_EDK_INC}/${dir})
    endif()

    # Use destination parent, as the last part of the source directory is copied as well
    cmake_path(GET dest PARENT_PATH dest_p)

    file(MAKE_DIRECTORY ${dest_p})
    file(COPY ${dir} DESTINATION ${dest_p} FILES_MATCHING PATTERN "*.h")

    cmake_path(RELATIVE_PATH dest BASE_DIRECTORY ${LLEXT_EDK} OUTPUT_VARIABLE dest_rel)
    list(APPEND all_flags_make "-I\$(${install_dir_var})/${dest_rel}")
    list(APPEND all_flags_cmake "-I\${${install_dir_var}}/${dest_rel}")
endforeach()

list(JOIN all_flags_make " " all_flags_str)
file(WRITE ${LLEXT_EDK}/Makefile.cflags "LLEXT_CFLAGS = ${all_flags_str}")

list(JOIN all_flags_cmake " " all_flags_str)
file(WRITE ${LLEXT_EDK}/cmake.cflags "set(LLEXT_CFLAGS ${all_flags_str})")

file(ARCHIVE_CREATE
    OUTPUT ${LLEXT_EDK_FILE}
    PATHS ${LLEXT_EDK}
    FORMAT gnutar
    COMPRESSION XZ
)

file(REMOVE_RECURSE ${LLEXT_EDK})
