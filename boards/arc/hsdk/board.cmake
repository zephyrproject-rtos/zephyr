# SPDX-License-Identifier: Apache-2.0

# TODO: can this board just use the usual openocd runner?
set(BOARD_FLASH_RUNNER em-starterkit)
set(BOARD_DEBUG_RUNNER em-starterkit)
board_finalize_runner_args(em-starterkit)
