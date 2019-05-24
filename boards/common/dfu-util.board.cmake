# SPDX-License-Identifier: Apache-2.0

set_ifndef(BOARD_FLASH_RUNNER dfu-util)
board_finalize_runner_args(dfu-util) # No default arguments to provide.
