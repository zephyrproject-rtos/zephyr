# Zephyr code can configure itself based on a KConfig'uration with the
# header file autoconf.h. There exists an analogous file
# generated_dts_board.h that allows configuration based on information
# encoded in DTS.
#
# Here we call on dtc, the gcc preprocessor, and
# scripts/dts/extract_dts_includes.py to generate this header file at
# CMake configure-time.
#
# See ~/zephyr/doc/dts
set(GENERATED_DTS_BOARD_H    ${PROJECT_BINARY_DIR}/include/generated/generated_dts_board.h)
set(GENERATED_DTS_BOARD_CONF ${PROJECT_BINARY_DIR}/include/generated/generated_dts_board.conf)
set_ifndef(DTS_SOURCE ${BOARD_DIR}/${BOARD}.dts)
set_ifndef(DTS_COMMON_OVERLAYS ${ZEPHYR_BASE}/dts/common/common.dts)
set_ifndef(DTS_APP_BINDINGS ${APPLICATION_SOURCE_DIR}/dts/bindings)
set_ifndef(DTS_APP_INCLUDE ${APPLICATION_SOURCE_DIR}/dts)

set(dts_files
  ${DTS_SOURCE}
  ${DTS_COMMON_OVERLAYS}
  ${shield_dts_files}
  )

if(CONFIG_HAS_DTS)

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

    math(EXPR i "${i}+1")
  endforeach()

  # TODO: Cut down on CMake configuration time by avoiding
  # regeneration of generated_dts_board.h on every configure. How
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
    -isystem ${DTS_APP_INCLUDE}
    -isystem ${ZEPHYR_BASE}/include
    -isystem ${ZEPHYR_BASE}/dts/${ARCH}
    -isystem ${ZEPHYR_BASE}/dts
    -include ${AUTOCONF_H}
    ${DTC_INCLUDE_FLAG_FOR_DTS}  # include the DTS source and overlays
    -I${ZEPHYR_BASE}/dts/common
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
  execute_process(
    COMMAND ${DTC}
    -O dts
    -o ${BOARD}.dts_compiled
    -b 0
    -E unit_address_vs_reg
    ${EXTRA_DTC_FLAGS} # User settable
    ${BOARD}.dts.pre.tmp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  # Error-out when the deprecated naming convention is found (until
  # after 1.14.0 has been released)
  foreach(path
    ${BOARD_DIR}/dts.fixup
    ${PROJECT_SOURCE_DIR}/soc/${ARCH}/${SOC_PATH}/dts.fixup
      ${APPLICATION_SOURCE_DIR}/dts.fixup
    )
    if(EXISTS ${path})
      message(FATAL_ERROR
      "A deprecated filename has been detected. Porting is required."
      "The file '${path}' exists, but it should be named dts_fixup.h instead."
      "See https://github.com/zephyrproject-rtos/zephyr/pull/10352 for more details"
      )
    endif()
  endforeach()

  # Run extract_dts_includes.py for the header file
  # generated_dts_board.h
  set_ifndef(DTS_BOARD_FIXUP_FILE ${BOARD_DIR}/dts_fixup.h)
  set_ifndef(DTS_SOC_FIXUP_FILE   ${SOC_DIR}/${ARCH}/${SOC_PATH}/dts_fixup.h)

  list(APPEND dts_fixups
    ${DTS_BOARD_FIXUP_FILE}
    ${DTS_SOC_FIXUP_FILE}
    ${APPLICATION_SOURCE_DIR}/dts_fixup.h
    ${shield_dts_fixups}
    )

  foreach(fixup ${dts_fixups})
    if(EXISTS ${fixup})
      list(APPEND existing_dts_fixups ${fixup})
    endif()
  endforeach()

  if("${existing_dts_fixups}" STREQUAL "")
    unset(DTS_FIXUPS_WITH_FLAG)
  else()
    set(DTS_FIXUPS_WITH_FLAG --fixup ${existing_dts_fixups})
  endif()

  if(NOT EXISTS ${DTS_APP_BINDINGS})
    set(DTS_APP_BINDINGS)
  endif()

  set(CMD_EXTRACT_DTS_INCLUDES ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/dts/extract_dts_includes.py
    --dts ${BOARD}.dts_compiled
    --yaml ${ZEPHYR_BASE}/dts/bindings ${DTS_APP_BINDINGS}
    ${DTS_FIXUPS_WITH_FLAG}
    --keyvalue ${GENERATED_DTS_BOARD_CONF}
    --include ${GENERATED_DTS_BOARD_H}
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
  file(WRITE ${GENERATED_DTS_BOARD_H} "/* WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY! */")
endif()
