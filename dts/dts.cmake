# Zephyr code can configure itself based on a KConfig'uration with the
# header file autoconf.h. There exists an analogous file
# generated_dts_board.h that allows configuration based on information
# encoded in DTS.
#
# Here we call on dtc, the gcc preprocessor, and
# scripts/extract_dts_includes.py to generate this header file at
# CMake configure-time.
#
# See ~/zephyr/doc/dts
set(GENERATED_DTS_BOARD_H    ${PROJECT_BINARY_DIR}/include/generated/generated_dts_board.h)
set(GENERATED_DTS_BOARD_CONF ${PROJECT_BINARY_DIR}/include/generated/generated_dts_board.conf)
set(DTS_SOURCE ${PROJECT_SOURCE_DIR}/dts/${ARCH}/${BOARD_NAME}.dts)

message(STATUS "Generating zephyr/include/generated/generated_dts_board.h")

if(CONFIG_HAS_DTS)
  # TODO: Cut down on CMake configuration time by avoiding
  # regeneration of generated_dts_board.h on every configure. How
  # challenging is this? What are the dts dependencies? We run the
  # preprocessor, and it seems to be including all kinds of
  # directories with who-knows how many header files.

  # Run the C preprocessor on the .dts source file to create the
  # intermediary file *.dts.pre.tmp
  execute_process(
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp 
    -nostdinc 
    ${ZEPHYR_INCLUDES} 
    -I${PROJECT_SOURCE_DIR}/arch/${ARCH}/source_group 
    -isystem ${PROJECT_SOURCE_DIR}/dts/${ARCH} 
    -isystem ${PROJECT_SOURCE_DIR}/dts 
    -include ${AUTOCONF_H} 
    -I${PROJECT_SOURCE_DIR}/dts/common 
    -I${PROJECT_SOURCE_DIR}/drivers/of/testcase-data 
    -undef -D__DTS__
    -P
    -E ${DTS_SOURCE} 
    -o ${BOARD_NAME}.dts.pre.tmp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )

  # Run the DTC on *.dts.pre.tmp to create the intermediary file *.dts_compiled
  execute_process(
    COMMAND ${DTC}
    -O dts
    -o ${BOARD_NAME}.dts_compiled
    -b 0
    ${BOARD_NAME}.dts.pre.tmp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )

  # Run extract_dts_includes.py for the header file
  # generated_dts_board.h
  set(FIXUP_FILE ${PROJECT_SOURCE_DIR}/dts/${ARCH}/${BOARD_NAME}.fixup)
  if(EXISTS ${FIXUP_FILE})
    set(FIXUP -f ${FIXUP_FILE})
  endif()
  set(CMD_EXTRACT_DTS_INCLUDES ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/extract_dts_includes.py
    --dts ${BOARD_NAME}.dts_compiled
    --yaml ${PROJECT_SOURCE_DIR}/dts/${ARCH}/yaml
    ${FIXUP}
    )
  execute_process(
    COMMAND ${CMD_EXTRACT_DTS_INCLUDES}
    OUTPUT_VARIABLE STDOUT
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )

  # extract_dts_includes.py writes the header file contents to stdout,
  # which we capture in the variable STDOUT and then finaly write into
  # the header file.
  file(WRITE ${GENERATED_DTS_BOARD_H} "${STDOUT}" )

  # Run extract_dts_includes.py to create a .conf file that can be
  # included into the CMake namespace
  execute_process(
    COMMAND ${CMD_EXTRACT_DTS_INCLUDES} --keyvalue
    OUTPUT_VARIABLE STDOUT
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
  file(WRITE ${GENERATED_DTS_BOARD_CONF} "${STDOUT}" )
  import_kconfig(${GENERATED_DTS_BOARD_CONF})

else()
  file(WRITE ${GENERATED_DTS_BOARD_H} "/* WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY! */")
endif()
