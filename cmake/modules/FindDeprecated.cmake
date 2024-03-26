# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

# FindDeprecated module provides a single location for deprecated CMake build code.
# Whenever CMake code is deprecated it should be moved to this module and
# corresponding COMPONENTS should be created with name identifying the deprecated code.
#
# This makes it easier to maintain deprecated code and cleanup such code when it
# has been deprecated for two releases.
#
# Example:
# CMakeList.txt contains deprecated code, like:
# if(DEPRECATED_VAR)
#   deprecated()
# endif()
#
# such code can easily be around for a long time, so therefore such code should
# be moved to this module and can then be loaded as:
# FindDeprecated.cmake
# if(<deprecated_name> IN_LIST Deprecated_FIND_COMPONENTS)
#   # This code has been deprecated after Zephyr x.y
#   if(DEPRECATED_VAR)
#     deprecated()
#   endif()
# endif()
#
# and then in the original CMakeLists.txt, this code is inserted instead:
# find_package(Deprecated COMPONENTS <deprecated_name>)
#
# The module defines the following variables:
#
# 'Deprecated_FOUND', 'DEPRECATED_FOUND'
# True if the Deprecated component was found and loaded.

if("${Deprecated_FIND_COMPONENTS}" STREQUAL "")
  message(WARNING "find_package(Deprecated) missing required COMPONENTS keyword")
endif()

if("CROSS_COMPILE" IN_LIST Deprecated_FIND_COMPONENTS)
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS CROSS_COMPILE)
  # This code was deprecated after Zephyr v3.1.0
  if(NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT)
    set(ZEPHYR_TOOLCHAIN_VARIANT $ENV{ZEPHYR_TOOLCHAIN_VARIANT})
  endif()

  if(NOT ZEPHYR_TOOLCHAIN_VARIANT AND
     (CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE})))
      set(ZEPHYR_TOOLCHAIN_VARIANT cross-compile CACHE STRING "Zephyr toolchain variant" FORCE)
      message(DEPRECATION  "Setting CROSS_COMPILE without setting ZEPHYR_TOOLCHAIN_VARIANT is deprecated."
                           "Please set ZEPHYR_TOOLCHAIN_VARIANT to 'cross-compile'"
      )
  endif()
endif()

if("XTOOLS" IN_LIST Deprecated_FIND_COMPONENTS)
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS XTOOLS)
  # This code was deprecated after Zephyr v3.3.0
  # When removing support for `xtools`, remember to also remove:
  # cmake/toolchain/xtools (folder with files)
  # doc/develop/toolchains/crosstool_ng.rst and update the index.rst file.
  message(DEPRECATION "XTOOLS toolchain variant is deprecated. "
                      "Please set ZEPHYR_TOOLCHAIN_VARIANT to 'zephyr'")
endif()

if("SPARSE" IN_LIST Deprecated_FIND_COMPONENTS)
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS SPARSE)
  # This code was deprecated after Zephyr v3.2.0
  if(SPARSE)
    message(DEPRECATION
        "Setting SPARSE='${SPARSE}' is deprecated. "
        "Please set ZEPHYR_SCA_VARIANT to 'sparse'"
    )
    if("${SPARSE}" STREQUAL "y")
      set_ifndef(ZEPHYR_SCA_VARIANT sparse)
    endif()
  endif()
endif()

if("SOURCES" IN_LIST Deprecated_FIND_COMPONENTS)
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS SOURCES)
  if(SOURCES)
    message(DEPRECATION
        "Setting SOURCES prior to calling find_package() for unit tests is deprecated."
        " To add sources after find_package() use:\n"
        "    target_sources(testbinary PRIVATE <source-file.c>)")
  endif()
endif()

if("PYTHON_PREFER" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v3.4.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS PYTHON_PREFER)
  if(DEFINED PYTHON_PREFER)
    message(DEPRECATION "'PYTHON_PREFER' variable is deprecated. Please use "
                        "Python3_EXECUTABLE instead.")
    if(NOT DEFINED Python3_EXECUTABLE)
      set(Python3_EXECUTABLE ${PYTHON_PREFER})
    endif()
  endif()
endif()

if("SEARCHED_LINKER_SCRIPT" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v3.5.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS SEARCHED_LINKER_SCRIPT)

  # Try a board specific linker file
  set(LINKER_SCRIPT ${BOARD_DIR}/linker.ld)
  if(NOT EXISTS ${LINKER_SCRIPT})
    # If not available, try an SoC specific linker file
    set(LINKER_SCRIPT ${SOC_FULL_DIR}/linker.ld)
  endif()
  message(DEPRECATION
      "Pre-defined `linker.ld` script is deprecated. Please set "
      "BOARD_LINKER_SCRIPT or SOC_LINKER_SCRIPT to point to ${LINKER_SCRIPT} "
      "or one of the Zephyr provided common linker scripts for the ${ARCH} "
      "architecture."
  )
endif()

if("ZEPHYR_FILE_DEPRECATED_OVERLAYS" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v3.6.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS ZEPHYR_FILE_DEPRECATED_OVERLAYS)
  if(DEFINED ZFILE_BOARD_QUALIFIERS)

    if(ZFILE_NAMES)
      # If NAMES are specified then only look for deprecated file names if directly specified.
      set(dts_filename_list ${ZFILE_DEPRECATED})
    else()
      # Include board name without SoC as deprecated option
      string(REGEX REPLACE "^/[^/]*(.*)" "\\1" qualifiers_without_soc "${ZFILE_BOARD_QUALIFIERS}")
      string(REPLACE "/" "_" qualifiers_without_soc "${qualifiers_without_soc}")

      zephyr_build_string(dts_filename_list
                          BOARD ${ZFILE_BOARD}
                          BOARD_REVISION ${ZFILE_BOARD_REVISION}
                          BOARD_QUALIFIERS ${qualifiers_without_soc}
                          BUILD ${ZFILE_BUILD}
                          MERGE REVERSE
      )

      list(TRANSFORM dts_filename_list APPEND ".overlay")
    endif()

    foreach(path ${ZFILE_CONF_FILES})
      foreach(filename ${dts_filename_list})
        if(NOT IS_ABSOLUTE ${filename})
          set(test_file ${path}/${filename})
        else()
          set(test_file ${filename})
        endif()
        zephyr_file_suffix(test_file SUFFIX ${ZFILE_SUFFIX})

        if(EXISTS ${test_file})
          list(APPEND found_dts_files ${test_file})

          message(DEPRECATION
                  "Overlay files (${filename}) without the SoC name are "
                  "deprecated after Zephyr 3.6, you should use <board>_<soc>.overlay instead")
        endif()

        if(DEFINED ZFILE_BUILD)
          set(deprecated_file_found y)
        endif()
      endforeach()
    endforeach()
  endif()
endif()

if("ZEPHYR_FILE_DEPRECATED_CONF" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v3.6.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS ZEPHYR_FILE_DEPRECATED_CONF)
  if(DEFINED ZFILE_BOARD_QUALIFIERS)
    # Include board name without SoC as deprecated option
    string(REGEX REPLACE "^/[^/]*(.*)" "\\1" qualifiers_without_soc "${ZFILE_BOARD_QUALIFIERS}")
    string(REPLACE "/" "_" qualifiers_without_soc "${qualifiers_without_soc}")

    zephyr_build_string(conf_filename_list
                        BOARD ${ZFILE_BOARD}
                        BOARD_REVISION ${ZFILE_BOARD_REVISION}
                        BOARD_QUALIFIERS ${qualifiers_without_soc}
                        BUILD ${ZFILE_BUILD}
                        MERGE REVERSE
    )

    list(TRANSFORM conf_filename_list APPEND ".conf")

    foreach(path ${ZFILE_CONF_FILES})
      foreach(filename ${conf_filename_list})
        if(NOT IS_ABSOLUTE ${filename})
          set(test_file ${path}/${filename})
        else()
          set(test_file ${filename})
        endif()
        zephyr_file_suffix(test_file SUFFIX ${ZFILE_SUFFIX})

        if(EXISTS ${test_file})
          list(APPEND found_conf_files ${test_file})

          message(DEPRECATION
                  "Kconfig fragment files (${filename}) without the SoC name are "
                  " deprecated after Zephyr 3.6, you should use <board>_<soc>.conf instead")
        endif()

        if(DEFINED ZFILE_BUILD)
          set(deprecated_file_found y)
        endif()
      endforeach()
    endforeach()
  endif()
endif()

if("ZEPHYR_FILE_DEPRECATED_DEFCONF" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v3.6.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS ZEPHYR_FILE_DEPRECATED_DEFCONF)
  if(DEFINED ZFILE_BOARD_QUALIFIERS)
    # Include board name without SoC as deprecated option
    string(REGEX REPLACE "^/[^/]*(.*)" "\\1" qualifiers_without_soc "${ZFILE_BOARD_QUALIFIERS}")
    string(REPLACE "/" "_" qualifiers_without_soc "${qualifiers_without_soc}")

    zephyr_build_string(defconf_filename_list
                        BOARD ${ZFILE_BOARD}
                        BOARD_REVISION ${ZFILE_BOARD_REVISION}
                        BOARD_QUALIFIERS ${qualifiers_without_soc}
                        MERGE REVERSE
    )

    list(TRANSFORM defconf_filename_list APPEND "_defconfig")

    foreach(path ${ZFILE_CONF_FILES})
      foreach(filename ${defconf_filename_list})
        if(NOT IS_ABSOLUTE ${filename})
          set(test_file ${path}/${filename})
        else()
          set(test_file ${filename})
        endif()

        if(EXISTS ${test_file})
          list(APPEND found_defconf_files ${test_file})

          message(DEPRECATION
                  "defconfig files (${filename}) without the SoC name are "
                  " deprecated after Zephyr 3.6, you should use <board>_<soc>_defconfig instead")
        endif()
      endforeach()
    endforeach()
  endif()
endif()

if("DTS_SOURCE" IN_LIST Deprecated_FIND_COMPONENTS)
  # This code was deprecated after Zephyr v3.6.0
  list(REMOVE_ITEM Deprecated_FIND_COMPONENTS DTS_SOURCE)
  if(DEFINED BOARD_QUALIFIERS)
    # Include board name without SoC as deprecated option
    string(REGEX REPLACE "^/[^/]*(.*)" "\\1" qualifiers_without_soc "${BOARD_QUALIFIERS}")
    string(REPLACE "/" "_" qualifiers_without_soc "${qualifiers_without_soc}")
    zephyr_build_string(board_string
                        BOARD ${BOARD}
                        BOARD_QUALIFIERS ${qualifiers_without_soc}
    )

    if(EXISTS ${BOARD_DIR}/${board_string}.dts)
      set(DTS_SOURCE ${BOARD_DIR}/${board_string}.dts)
      message(DEPRECATION
              "Devicetree files (${board_string}.dts) without the SoC name are deprecated"
              " after Zephyr 3.6, you should use <board>_<soc>.dts instead"
      )
    endif()
  endif()
endif()

if(NOT "${Deprecated_FIND_COMPONENTS}" STREQUAL "")
  message(STATUS "The following deprecated component(s) could not be found: "
                 "${Deprecated_FIND_COMPONENTS}")
endif()

set(Deprecated_FOUND True)
set(DEPRECATED_FOUND True)
