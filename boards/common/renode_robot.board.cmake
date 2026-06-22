# SPDX-License-Identifier: Apache-2.0

board_set_robot_runner_ifnset(renode-robot)

# `--variable` is a renode-test argument, for setting a variable that can be later used in a .robot file:
# ELF:         used in common.robot to set the `elf` variable in the default .resc script defined in board.cmake
# RESC:        path to the .resc script, defined in board.cmake
# UART:        default UART used by Robot in tests, defined in board.cmake
# KEYWORDS:    path to common.robot, which contains common Robot keywords
# RESULTS_DIR: directory in which Robot artifacts will be generated after running a testsuite
board_runner_args(renode-robot "--renode-robot-arg=--variable=ELF:@${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}")
board_runner_args(renode-robot "--renode-robot-arg=--variable=RESC:@${RENODE_SCRIPT}")
board_runner_args(renode-robot "--renode-robot-arg=--variable=UART:${RENODE_UART}")
board_runner_args(renode-robot "--renode-robot-arg=--variable=KEYWORDS:${ZEPHYR_BASE}/tests/robot/common.robot")
board_runner_args(renode-robot "--renode-robot-arg=--variable=RESULTS_DIR:${APPLICATION_BINARY_DIR}")

board_finalize_runner_args(renode-robot)
