# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS nsim)

string(SUBSTRING "${BOARD_QUALIFIERS}" 1 -1 NSIM_BASE_FILENAME)
string(REPLACE "/" "_" NSIM_BASE_FILENAME "${NSIM_BASE_FILENAME}")

board_set_flasher_ifnset(arc-nsim)
board_set_debugger_ifnset(arc-nsim)

set(NSIM_PROPS "${NSIM_BASE_FILENAME}.props")
board_runner_args(arc-nsim "--props=${NSIM_PROPS}")

string(REPLACE "nsim" "mdb" MDB_ARGS "${NSIM_BASE_FILENAME}.args")

board_runner_args(mdb-nsim "--cores=${CONFIG_MP_MAX_NUM_CPUS}" "--nsim_args=${MDB_ARGS}")
board_runner_args(mdb-hw "--cores=${CONFIG_MP_MAX_NUM_CPUS}")

board_finalize_runner_args(arc-nsim)
include(${ZEPHYR_BASE}/boards/common/mdb-nsim.board.cmake)
include(${ZEPHYR_BASE}/boards/common/mdb-hw.board.cmake)
