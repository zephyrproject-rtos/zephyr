# SPDX-License-Identifier: Apache-2.0

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
  ${MDB} -pset=2 -psetname=core1 -prop=ident=0x00000150 -cmpd=soc
  @${BOARD_DIR}/support/${MDB_ARGS} ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME} &&
  NSIM_MULTICORE=1 ${MDB} -multifiles=core0,core1 -cmpd=soc -run -cl
  DEPENDS ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  USES_TERMINAL
  )
