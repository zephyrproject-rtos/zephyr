# Copyright (c) 2020 Erik Larson
# Copyright (c) 2020 Friedt Professional Engineering Services, Inc
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=CC1352R1F3" "--iface=jtag" "--tool-opt=-jtagconf -1,-1 -autoconnect 1")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
