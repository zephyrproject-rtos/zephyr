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
  -e '$$bin=@${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}\; include @${RENODE_SCRIPT}\; ${RENODE_OVERLAY} s'
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  DEPENDS ${logical_target_for_zephyr_elf}
  USES_TERMINAL
  )

find_program(
  RENODE_TEST
  renode-test
  )

set(RENODE_TEST_FLAGS
  --variable ELF:@${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  --variable RESC:@${RENODE_SCRIPT}
  --variable UART:${RENODE_UART}
  --variable KEYWORDS:${ZEPHYR_BASE}/tests/robot/common.robot
  --results-dir ${APPLICATION_BINARY_DIR}
  )

add_custom_target(run_renode_test
  COMMAND /bin/sh -c "\
    if [ -z $$ROBOT_FILES ] \;\
    then\
        echo ''\;\
        echo '--- Error: Robot file path is required to run Robot tests in Renode. To provide the path please set the ROBOT_FILES variable.'\;\
        echo '--- To rerun the test with west execute:'\;\
        echo '--- ROBOT_FILES=\\<FILES\\> west build -p -b \\<BOARD\\> -s \\<SOURCE_DIR\\> -t run_renode_test'\;\
        echo ''\;\
        exit 1\;\
    fi\;"
  COMMAND
  ${RENODE_TEST}
  ${RENODE_TEST_FLAGS}
  ${APPLICATION_SOURCE_DIR}/$$ROBOT_FILES
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  DEPENDS ${logical_target_for_zephyr_elf}
  USES_TERMINAL
  )
