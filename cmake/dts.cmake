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
set_ifndef(DTS_SOURCE ${BOARD_ROOT}/boards/${ARCH}/${BOARD_FAMILY}/${BOARD}.dts)
set_ifndef(DTS_COMMON_OVERLAYS ${ZEPHYR_BASE}/dts/common/common.dts)
set_ifndef(DTS_APP_BINDINGS ${APPLICATION_SOURCE_DIR}/dts/bindings)

message(STATUS "Generating zephyr/include/generated/generated_dts_board.h")

if(CONFIG_HAS_DTS)

  if(DTC_OVERLAY_FILE)
    # Convert from space-separated files into file list
    string(REPLACE " " ";" DTC_OVERLAY_FILE_AS_LIST ${DTC_OVERLAY_FILE})
  endif()

  # Prepend common overlays
  set(DTC_OVERLAY_FILE_AS_LIST ${DTS_COMMON_OVERLAYS} ${DTC_OVERLAY_FILE_AS_LIST})

  set(
    dts_files
    ${DTS_SOURCE}
    ${DTC_OVERLAY_FILE_AS_LIST}
  )

  unset(DTC_INCLUDE_FLAG_FOR_DTS)
  foreach(dts_file ${dts_files})
    list(APPEND DTC_INCLUDE_FLAG_FOR_DTS
         -include ${dts_file})
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
    -I${ZEPHYR_BASE}/soc/${ARCH}
    -isystem ${ZEPHYR_BASE}/include
    -isystem ${ZEPHYR_BASE}/dts/${ARCH}
    -isystem ${ZEPHYR_BASE}/dts
    -include ${AUTOCONF_H}
    ${DTC_INCLUDE_FLAG_FOR_DTS}  # include the DTS source and overlays
    -I${ZEPHYR_BASE}/dts/common
    -I${ZEPHYR_BASE}/drivers
    -undef -D__DTS__
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
    ${BOARD}.dts.pre.tmp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  # Run extract_dts_includes.py for the header file
  # generated_dts_board.h
  set_ifndef(DTS_BOARD_FIXUP_FILE ${BOARD_ROOT}/boards/${ARCH}/${BOARD_FAMILY}/dts.fixup)
  if(EXISTS ${DTS_BOARD_FIXUP_FILE})
    set(DTS_BOARD_FIXUP ${DTS_BOARD_FIXUP_FILE})
  endif()
  set_ifndef(DTS_SOC_FIXUP_FILE ${SOC_DIR}/${ARCH}/${SOC_PATH}/dts.fixup)
  if(EXISTS ${DTS_SOC_FIXUP_FILE})
    set(DTS_SOC_FIXUP ${DTS_SOC_FIXUP_FILE})
  endif()
  if(EXISTS ${APPLICATION_SOURCE_DIR}/dts.fixup)
    set(DTS_APP_FIXUP ${APPLICATION_SOURCE_DIR}/dts.fixup)
  endif()

  set(DTS_FIXUPS ${DTS_SOC_FIXUP} ${DTS_BOARD_FIXUP} ${DTS_APP_FIXUP})
  if(NOT "${DTS_FIXUPS}" STREQUAL "")
    set(DTS_FIXUPS --fixup ${DTS_FIXUPS})
  endif()

  if(NOT EXISTS ${DTS_APP_BINDINGS})
    set(DTS_APP_BINDINGS)
  endif()

  set(CMD_EXTRACT_DTS_INCLUDES ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/dts/extract_dts_includes.py
    --dts ${BOARD}.dts_compiled
    --yaml ${ZEPHYR_BASE}/dts/bindings ${DTS_APP_BINDINGS}
    ${DTS_FIXUPS}
    --keyvalue ${GENERATED_DTS_BOARD_CONF}
    --include ${GENERATED_DTS_BOARD_H}
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

  import_kconfig(${GENERATED_DTS_BOARD_CONF})

else()
  file(WRITE ${GENERATED_DTS_BOARD_H} "/* WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY! */")
endif()
