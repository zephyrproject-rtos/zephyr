# SPDX-License-Identifier: Apache-2.0

find_program(
  RENODE
  renode
  )

set(RENODE_FLAGS
  --disable-xwt
  --pid-file renode.pid
  )

add_custom_target(run
  COMMAND
  ${RENODE}
  ${RENODE_FLAGS}
  -e '$$bin=@${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}\; include @${RENODE_SCRIPT}\; s'
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  USES_TERMINAL
  )
