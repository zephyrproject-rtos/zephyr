# SPDX-License-Identifier: Apache-2.0

set_ifndef(BOARD_FLASH_RUNNER bossac)
board_finalize_runner_args(bossac "--bossac=${BOSSAC}")
