# SPDX-License-Identifier: Apache-2.0
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
if(BOARD STREQUAL "artemis_nano")
    board_runner_args(sp_artemis "--baud-rate=115200" "--serial-port=/dev/ttyUSB0")
    include(${ZEPHYR_BASE}/boards/common/sp_artemis.board.cmake)
endif()
>>>>>>> d737658ec14 (before modification for west flash)
>>>>>>> a53dd08c5d9 (fixing the west flash)

board_runner_args(pyocd "--target=cortex_m" "--frequency=4000000" "--probe=/dev/ttyUSB0")

include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
include(${ZEPHYR_BASE}/boards/common/trace32.board.cmake)
