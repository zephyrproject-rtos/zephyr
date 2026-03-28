# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

# options after "--tool-opt=" are directly passed to the tool. So instead of "--iface=JTAG" you could also write "--tool-opt=-if JTAG"
board_runner_args(jlink "--device=R5F52618" "--iface=FINE" "--speed=1000" "--tool-opt=-jtagconf -1,-1 -autoconnect 1" )
board_runner_args(rfp "--device=RX200" "--tool=e2l" "--interface=fine" "--erase")

include(${ZEPHYR_BASE}/boards/common/rfp.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
