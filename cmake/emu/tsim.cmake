# SPDX-License-Identifier: Apache-2.0

set(TSIM_FLAGS -e run -e exit)

add_custom_target(run
  COMMAND
  ${TSIM}
  ${TSIM_SYS}
  ${TSIM_FLAGS}
  ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  USES_TERMINAL
  )
