# SPDX-License-Identifier: Apache-2.0

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include/generated)

# Zephyr code can configure itself based on a KConfig'uration with the
# header file autoconf.h. There exists an analogous file
# generated_dts_board_unfixed.h that allows configuration based on information
# encoded in DTS.
#
# Here we call on dtc, the gcc preprocessor, and
# scripts/dts/extract_dts_includes.py to generate this header file at
# CMake configure-time.
#
# See ~/zephyr/doc/dts
set(GENERATED_DTS_BOARD_UNFIXED_H ${PROJECT_BINARY_DIR}/include/generated/generated_dts_board_unfixed.h)
set(GENERATED_DTS_BOARD_CONF      ${PROJECT_BINARY_DIR}/include/generated/generated_dts_board.conf)

set_ifndef(DTS_SOURCE ${BOARD_DIR}/${BOARD}.dts)
set_ifndef(DTS_COMMON_OVERLAYS ${ZEPHYR_BASE}/dts/common/common.dts)

# 'DTS_ROOT' is a list of directories where a directory tree with DT
# files may be found. It always includes the application directory and
# ${ZEPHYR_BASE}.
list(APPEND
  DTS_ROOT
  ${APPLICATION_SOURCE_DIR}
  ${ZEPHYR_BASE}
  )

set(dts_files
  ${DTS_SOURCE}
  ${DTS_COMMON_OVERLAYS}
  ${shield_dts_files}
  )

# TODO: What to do about non-posix platforms where NOT CONFIG_HAS_DTS (xtensa)?
# Drop support for NOT CONFIG_HAS_DTS perhaps?
if(EXISTS ${DTS_SOURCE})
  set(SUPPORTS_DTS 1)
else()
  set(SUPPORTS_DTS 0)
endif()

if(SUPPORTS_DTS)
  if(DTC_OVERLAY_FILE)
    # Convert from space-separated files into file list
    string(REPLACE " " ";" DTC_OVERLAY_FILE_AS_LIST ${DTC_OVERLAY_FILE})
    list(APPEND
      dts_files
      ${DTC_OVERLAY_FILE_AS_LIST}
      )
  endif()

  set(i 0)
  unset(DTC_INCLUDE_FLAG_FOR_DTS)
  foreach(dts_file ${dts_files})
    list(APPEND DTC_INCLUDE_FLAG_FOR_DTS
         -include ${dts_file})

    if(i EQUAL 0)
      message(STATUS "Loading ${dts_file} as base")
    else()
      message(STATUS "Overlaying ${dts_file}")
    endif()

    # Ensure that changes to 'dts_file's cause CMake to be re-run
    set_property(DIRECTORY APPEND PROPERTY
      CMAKE_CONFIGURE_DEPENDS
      ${dts_file}
      )

    math(EXPR i "${i}+1")
  endforeach()

  foreach(dts_root ${DTS_ROOT})
    foreach(dts_root_path
        include
        dts/common
        dts/${ARCH}
        dts
        )
      set(full_path ${dts_root}/${dts_root_path})
      if(EXISTS ${full_path})
        list(APPEND
          DTS_ROOT_SYSTEM_INCLUDE_DIRS
          -isystem ${full_path}
          )
      endif()
    endforeach()
  endforeach()

  foreach(dts_root ${DTS_ROOT})
    set(full_path ${dts_root}/dts/bindings)
    if(EXISTS ${full_path})
      list(APPEND
        DTS_ROOT_BINDINGS
        ${full_path}
        )
    endif()
  endforeach()

  # TODO: Cut down on CMake configuration time by avoiding
  # regeneration of generated_dts_board_unfixed.h on every configure. How
  # challenging is this? What are the dts dependencies? We run the
  # preprocessor, and it seems to be including all kinds of
  # directories with who-knows how many header files.

  # Run the C preprocessor on an empty C source file that has one or
  # more DTS source files -include'd into it to create the
  # intermediary file *.dts.pre.tmp
  execute_process(
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp
    -nostdinc
    ${DTS_ROOT_SYSTEM_INCLUDE_DIRS}
    ${DTC_INCLUDE_FLAG_FOR_DTS}  # include the DTS source and overlays
    ${NOSYSDEF_CFLAG}
    -D__DTS__
    -P
    -E ${ZEPHYR_BASE}/misc/empty_file.c
    -o ${BOARD}.dts.pre.tmp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  # Run the DTC on *.dts.pre.tmp to create the intermediary file *.dts_compiled

  set(DTC_WARN_UNIT_ADDR_IF_ENABLED "")
  check_dtc_flag("-Wunique_unit_address_if_enabled" check)
  if (check)
    set(DTC_WARN_UNIT_ADDR_IF_ENABLED "-Wunique_unit_address_if_enabled")
  endif()
  set(DTC_NO_WARN_UNIT_ADDR "")
  check_dtc_flag("-Wno-unique_unit_address" check)
  if (check)
    set(DTC_NO_WARN_UNIT_ADDR "-Wno-unique_unit_address")
  endif()
  execute_process(
    COMMAND ${DTC}
    -O dts
    -o ${BOARD}.dts_compiled
    -b 0
    -E unit_address_vs_reg
    ${DTC_NO_WARN_UNIT_ADDR}
    ${DTC_WARN_UNIT_ADDR_IF_ENABLED}
    ${EXTRA_DTC_FLAGS} # User settable
    ${BOARD}.dts.pre.tmp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  set(CMD_EXTRACT_DTS_INCLUDES ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/dts/extract_dts_includes.py
    --dts ${BOARD}.dts_compiled
    --yaml ${DTS_ROOT_BINDINGS}
    --keyvalue ${GENERATED_DTS_BOARD_CONF}
    --include ${GENERATED_DTS_BOARD_UNFIXED_H}
    --old-alias-names
    )

  # Run extract_dts_includes.py to create a .conf and a header file that can be
  # included into the CMake namespace
  execute_process(
    COMMAND ${CMD_EXTRACT_DTS_INCLUDES}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  import_kconfig(CONFIG_ ${GENERATED_DTS_BOARD_CONF})
  import_kconfig(DT_     ${GENERATED_DTS_BOARD_CONF})

else()
  file(WRITE ${GENERATED_DTS_BOARD_UNFIXED_H} "/* WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY! */")
endif(SUPPORTS_DTS)
