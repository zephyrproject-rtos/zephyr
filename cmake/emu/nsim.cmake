# SPDX-License-Identifier: Apache-2.0
if("${BOARD_DEBUG_RUNNER}" STREQUAL "mdb-nsim" OR "${BOARD_FLASH_RUNNER}" STREQUAL "mdb-nsim")
# mdb is required to run nsim multicore targets
find_program(
  MDB
  mdb
  )
set(MDB_BASIC_OPTIONS -nooptions -nogoifmain -toggle=include_local_symbols=1)

if(${CONFIG_SOC_NSIM_HS_SMP})
 set(MDB_ARGS mdb_hs_smp.args)
elseif(${CONFIG_SOC_NSIM_EM})
 set(MDB_ARGS mdb_em.args)
elseif(${CONFIG_SOC_NSIM_EM7D_V22})
 set(MDB_ARGS mdb_em7d_v22.args)
elseif(${CONFIG_SOC_NSIM_SEM})
 set(MDB_ARGS mdb_sem.args)
elseif(${CONFIG_SOC_NSIM_HS})
 set(MDB_ARGS mdb_hs.args)
elseif(${CONFIG_SOC_NSIM_HS6X})
 set(MDB_ARGS mdb_hs6x.args)
endif()

# remove previous .sc.project folder which has temporary settings for MDB.
set(MDB_OPTIONS rm -rf ${APPLICATION_BINARY_DIR}/.sc.project)
if(CONFIG_MP_NUM_CPUS GREATER 1)
  set(MULTIFILES ${MDB} -multifiles=)
  foreach(val RANGE ${CONFIG_MP_NUM_CPUS})
    if(val LESS CONFIG_MP_NUM_CPUS)
      MATH(EXPR PSET_NUM "${CONFIG_MP_NUM_CPUS}-${val}")
      MATH(EXPR CORE_NUM "${CONFIG_MP_NUM_CPUS}-${val}-1")
      if (PSET_NUM GREATER 0)
        list(APPEND MDB_OPTIONS &&)
      endif()
      list(APPEND MDB_OPTIONS ${MDB} -pset=${PSET_NUM} -psetname=core${CORE_NUM})
      if (PSET_NUM GREATER 0)
        list(APPEND MDB_OPTIONS -prop=download=2)
        if (PSET_NUM GREATER 1)
          set(MULTIFILES ${MULTIFILES}core${CORE_NUM},)
        else()
          set(MULTIFILES ${MULTIFILES}core${CORE_NUM})
        endif()
      endif()
      list(APPEND MDB_OPTIONS ${MDB_BASIC_OPTIONS} -nsim @${BOARD_DIR}/support/${MDB_ARGS} ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME})
    endif()
  endforeach()
  list(APPEND MDB_OPTIONS && NSIM_MULTICORE=1 ${MULTIFILES} -run -cl)
else()
 list(APPEND MDB_OPTIONS && ${MDB} ${MDB_BASIC_OPTIONS} -nsim @${BOARD_DIR}/support/${MDB_ARGS} -run -cl)
endif()

add_custom_target(run
  COMMAND
  ${MDB_OPTIONS}
  ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  DEPENDS ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  USES_TERMINAL
)
else()
find_program(
  NSIM
  nsimdrv
  )

if(${CONFIG_SOC_NSIM_EM})
 set(NSIM_PROPS nsim_em.props)
elseif(${CONFIG_SOC_NSIM_EM7D_V22})
 set(NSIM_PROPS nsim_em7d_v22.props)
elseif(${CONFIG_SOC_NSIM_SEM})
 set(NSIM_PROPS nsim_sem.props)
elseif(${CONFIG_SOC_NSIM_HS})
 set(NSIM_PROPS nsim_hs.props)
elseif(${CONFIG_SOC_NSIM_HS6X})
 set(NSIM_PROPS nsim_hs6x.props)
endif()

add_custom_target(run
  COMMAND
  ${NSIM}
  -propsfile
  ${BOARD_DIR}/support/${NSIM_PROPS}
  ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  DEPENDS ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  USES_TERMINAL
  )

add_custom_target(debugserver
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
