# Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(rfp "--device=RX100" "--tool=e2l" "--interface=fine" "--erase")

include(${ZEPHYR_BASE}/boards/common/rfp.board.cmake)
