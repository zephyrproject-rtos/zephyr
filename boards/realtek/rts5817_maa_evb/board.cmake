# Copyright (c) 2024 Realtek Semiconductor, Inc.
# SPDX-License-Identifier: Apache-2.0
set_property(TARGET runners_yaml_props_target PROPERTY bin_file zephyr.rts5817.bin)

board_set_flasher_ifnset(rtsflash)
board_runner_args(rtsflash "--pid=0bda:5817" "--alt=0")
board_finalize_runner_args(rtsflash)
