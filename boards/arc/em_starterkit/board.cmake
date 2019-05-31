# SPDX-License-Identifier: Apache-2.0

# TODO: can this board just use the usual openocd runner?
board_set_flasher_ifnset(em-starterkit)
board_set_debugger_ifnset(em-starterkit)
board_finalize_runner_args(em-starterkit)
