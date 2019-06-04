# SPDX-License-Identifier: Apache-2.0

find_program(
  NSIM
  nsimdrv
  )

if(${CONFIG_SOC_NSIM_EM})
 set(NSIM_PROPS nsim_em.props)
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
