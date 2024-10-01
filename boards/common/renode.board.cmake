# SPDX-License-Identifier: Apache-2.0

board_set_sim_runner_ifnset(renode)

board_runner_args(renode "--renode-command=$elf=@${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}")
board_runner_args(renode "--renode-command=include @${RENODE_SCRIPT}")

board_finalize_runner_args(renode)
