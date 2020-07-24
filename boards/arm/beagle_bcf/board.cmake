# Copyright (c) 2020 Erik Larson
#
# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(misc-flasher)

# Install cc2538-bsl.py from https://github.com/jadonk/cc2538-bsl
board_finalize_runner_args(misc-flasher cc2538-bsl.py -e -w -v --bootloader-send-break --append /zephyr/zephyr.bin)
