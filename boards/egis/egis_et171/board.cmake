#
# Copyright (c) 2025 Egis Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
cmake_minimum_required(VERSION 3.20.0)

# To get more info about flashing, please contect https://www.egistec.com

# You must set the private key of the secure boot upgrade from env.
set(EGIS_FW_KEY $ENV{egis_fw_key})

# You can provider extension arguments for flashing firmware from env.
set(EXT_ARG $ENV{et171_fw_upgrade_ext_arg})

find_program(ET171_FLASHER NAMES et171_fw_upgrade)
board_set_flasher_ifnset(misc-flasher)
board_finalize_runner_args(misc-flasher
    ${ET171_FLASHER}
    ${EXT_ARG}
    -SCAK=${EGIS_FW_KEY}
    -FW="${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin"
)
