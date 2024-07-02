# SPDX-License-Identifier: Apache-2.0
if(BOARD STREQUAL "artemis_nano")
    board_runner_args(sp_artemis "--baud-rate=115200" "--serial-port=/dev/ttyUSB0")
    include(${ZEPHYR_BASE}/boards/common/sp_artemis.board.cmake)
endif()