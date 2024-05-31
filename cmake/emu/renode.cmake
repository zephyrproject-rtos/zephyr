# SPDX-License-Identifier: Apache-2.0

find_program(
  RENODE
  renode
  )

set(RENODE_FLAGS
  --disable-xwt
  --port -2
  --pid-file renode.pid
  )

# Check if there is any Renode script overlay defined for the target board
set(resc_overlay_file ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.resc)
if(EXISTS ${resc_overlay_file})
  set(RENODE_OVERLAY include "@${resc_overlay_file}\;")
  message(STATUS "Found Renode script overlay: ${resc_overlay_file}")
endif()

add_custom_target(run_renode
  COMMAND
  ${RENODE}
  ${RENODE_FLAGS}
  -e '$$elf=@${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}\; include @${RENODE_SCRIPT}\; ${RENODE_OVERLAY} s'
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  DEPENDS ${logical_target_for_zephyr_elf}
  USES_TERMINAL
  )
