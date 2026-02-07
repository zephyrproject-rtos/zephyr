# Copyright (c) 2025 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink
    "--device=ATSAMD21E18A"
    "--speed=4000"
)
board_runner_args(openocd
    "--cmd-pre-init=transport select swd"
    "--cmd-pre-init=source [find target/at91samdXX.cfg]"
)
board_runner_args(pyocd
    "--target=ATSAMD21E18A"
    "--frequency=400000"
)
board_runner_args(uf2
    "--board-id=SAMD21E18A-TRRSTrinkey-v0"
)

include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
