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

set(dts_files
  ${DTS_SOURCE}
  ${DTS_COMMON_OVERLAYS}
  )

#Parse boards/shields to generate the shield list

set(shield_dir ${ZEPHYR_BASE}/boards/shields)

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
foreach(shield_path ${shields_refs_list})
  get_filename_component(shield ${shield_path} NAME_WE)

  # Generate CONFIG flags matching each shield
  string(TOUPPER "CONFIG_SHIELD_${shield}" shield_config)

  if(${shield_config})
    # if shield config flag is on, add shield overlay to the shield overlays
    # list and dts.fixup file to the shield fixup file
    list(APPEND
      dts_files
      ${shield_dir}/${shield_path}
      )
    list(APPEND
      dts_fixups
      ${shield_dir}/${shield}/dts.fixup
      )
  endif()
endforeach()

message(STATUS "Generating zephyr/include/generated/generated_dts_board.h")

if(CONFIG_HAS_DTS)

  if(DTC_OVERLAY_FILE)
    # Convert from space-separated files into file list
    string(REPLACE " " ";" DTC_OVERLAY_FILE_AS_LIST ${DTC_OVERLAY_FILE})
    list(APPEND
      dts_files
      ${DTC_OVERLAY_FILE_AS_LIST}
      )
  endif()

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
  set_ifndef(DTS_BOARD_FIXUP_FILE       ${BOARD_ROOT}/boards/${ARCH}/${BOARD_FAMILY}/dts.fixup)
  set_ifndef(DTS_SOC_FIXUP_FILE   ${PROJECT_SOURCE_DIR}/soc/${ARCH}/${SOC_PATH}/dts.fixup)

  list(APPEND dts_fixups
    ${DTS_BOARD_FIXUP_FILE}
    ${DTS_SOC_FIXUP_FILE}
    ${APPLICATION_SOURCE_DIR}/dts.fixup
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
