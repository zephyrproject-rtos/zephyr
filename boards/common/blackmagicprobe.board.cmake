# SPDX-License-Identifier: Apache-2.0

set_ifndef(BOARD_FLASH_RUNNER blackmagicprobe)
set_ifndef(BOARD_DEBUG_RUNNER blackmagicprobe)
board_finalize_runner_args(blackmagicprobe) # No default arguments to provide
