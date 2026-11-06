# Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(rfp "--device=RH850/U2x"
    "--tool=e2"
    "--interface=csi"
    "--erase"
    )

include(${ZEPHYR_BASE}/boards/common/rfp.board.cmake)
