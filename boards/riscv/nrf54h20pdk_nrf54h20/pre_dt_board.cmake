# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Allow common DTS files to be included from the other board directory.
# To be removed after HWMv2 (#51831), once both directories can be merged into one.
string(REGEX REPLACE "/riscv/(.*$)" "/arm/\\1" BOARD_DIR_ARM "${BOARD_DIR}")
list(APPEND DTS_EXTRA_CPPFLAGS -isystem "${BOARD_DIR_ARM}")
