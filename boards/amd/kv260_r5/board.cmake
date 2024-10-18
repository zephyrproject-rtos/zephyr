# Copyright (c) 2022 Linaro.
#
# SPDX-License-Identifier: Apache-2.0
board_set_runner(FLASH xsdb)
board_set_debugger_ifnset(xsdb)
board_set_flasher_ifnset(xsdb)
board_finalize_runner_args(xsdb)
set(FLASH_RUNNER xsdb)
