# SPDX-License-Identifier: Apache-2.0
if(${CONFIG_SOC_NSIM_HS_SMP})
# mdb is required to run nsim multicore targets
find_program(
  MDB
  mdb
  )

if(${CONFIG_SOC_NSIM_HS_SMP})
 set(MDB_ARGS mdb_hs_smp.args)
endif()

add_custom_target(run
  COMMAND
  ${MDB} -pset=1 -psetname=core0 -prop=ident=0x00000050 -cmpd=soc
  @${BOARD_DIR}/support/${MDB_ARGS} ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME} &&
  ${MDB} -pset=2 -psetname=core1 -prop=download=2 -prop=ident=0x00000150 -cmpd=soc
  @${BOARD_DIR}/support/${MDB_ARGS} ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME} &&
  NSIM_MULTICORE=1 ${MDB} -multifiles=core0,core1 -cmpd=soc -run -cl
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
