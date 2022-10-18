# SPDX-License-Identifier: Apache-2.0
if("${BOARD_DEBUG_RUNNER}" STREQUAL "mdb-nsim" OR "${BOARD_FLASH_RUNNER}" STREQUAL "mdb-nsim")
# mdb is required to run nsim multicore targets
find_program(
  MDB
  mdb
  )
set(MDB_BASIC_OPTIONS -nooptions -nogoifmain -toggle=include_local_symbols=1)

# remove previous .sc.project folder which has temporary settings for MDB.
set(MDB_OPTIONS ${CMAKE_COMMAND} -E rm -rf ${APPLICATION_BINARY_DIR}/.sc.project)
if(CONFIG_MP_MAX_NUM_CPUS GREATER 1)
  set(MULTIFILES ${MDB} -multifiles=)
  foreach(val RANGE ${CONFIG_MP_MAX_NUM_CPUS})
    if(val LESS CONFIG_MP_MAX_NUM_CPUS)
      MATH(EXPR PSET_NUM "${CONFIG_MP_MAX_NUM_CPUS}-${val}")
      MATH(EXPR CORE_NUM "${CONFIG_MP_MAX_NUM_CPUS}-${val}-1")
      if(PSET_NUM GREATER 0)
        list(APPEND MDB_OPTIONS &&)
      endif()
      list(APPEND MDB_OPTIONS ${MDB} -pset=${PSET_NUM} -psetname=core${CORE_NUM})
      if(PSET_NUM GREATER 1)
        list(APPEND MDB_OPTIONS -prop=download=2)
        set(MULTIFILES ${MULTIFILES}core${CORE_NUM},)
      else()
        set(MULTIFILES ${MULTIFILES}core${CORE_NUM})
      endif()
      list(APPEND MDB_OPTIONS ${MDB_BASIC_OPTIONS} -nsim @${BOARD_DIR}/support/${MDB_ARGS} ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME})
    endif()
  endforeach()
  list(APPEND MDB_OPTIONS && NSIM_MULTICORE=1 ${MULTIFILES} -run -cl)
else()
 list(APPEND MDB_OPTIONS && ${MDB} ${MDB_BASIC_OPTIONS} -nsim @${BOARD_DIR}/support/${MDB_ARGS} -run -cl)
endif()

string(REPLACE ";" " " MDB_COMMAND "${MDB_OPTIONS}")
add_custom_target(run_nsim
  COMMAND
  ${MDB_OPTIONS}
  ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  DEPENDS ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  COMMENT "MDB COMMAND: ${MDB_COMMAND} ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}"
  USES_TERMINAL
)
else()
find_program(
  NSIM
  nsimdrv
  )

add_custom_target(run_nsim
  COMMAND
  ${NSIM}
  -propsfile
  ${BOARD_DIR}/support/${NSIM_PROPS}
  ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  DEPENDS ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  COMMENT "nSIM COMMAND: ${NSIM} -propsfile ${BOARD_DIR}/support/${NSIM_PROPS} ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}"
  USES_TERMINAL
  )

add_custom_target(debugserver_nsim
  COMMAND
  ${NSIM}
  -propsfile
  ${BOARD_DIR}/support/${NSIM_PROPS}
  -gdb -port=3333
  DEPENDS ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  USES_TERMINAL
  )
endif()
