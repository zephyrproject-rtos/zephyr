# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

# options after "--tool-opt=" are directly passed to the tool. So instead of "--iface=JTAG" you could also write "--tool-opt=-if JTAG"
board_runner_args(jlink "--device=R5F51308" "--iface=FINE" "--speed=1000" "--tool-opt=-jtagconf -1,-1 -autoconnect 1")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
