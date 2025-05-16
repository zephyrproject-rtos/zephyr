# Copyright (c) 2025 TQ-Systems GmbH <oss@ew.tq-group.com>
# SPDX-License-Identifier: Apache 2.0
# Author: Isaac L. L. Yuki

if(CONFIG_SOC_MIMXRT1176_CM7)
 board_runner_args(jlink "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()
