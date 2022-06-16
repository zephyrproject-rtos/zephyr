# SPDX-License-Identifier: Apache-2.0
set(SUPPORTED_EMU_PLATFORMS nsim)

if(NOT (CONFIG_SOC_NSIM_HS_SMP OR CONFIG_SOC_NSIM_HS6X_SMP OR CONFIG_SOC_NSIM_HS5X_SMP))
  board_set_flasher_ifnset(arc-nsim)
  board_set_debugger_ifnset(arc-nsim)

  set(NSIM_PROPS "${BOARD}.props")
  board_runner_args(arc-nsim "--props=${NSIM_PROPS}")
endif()

string(REPLACE "nsim" "mdb" MDB_ARGS "${BOARD}.args")

board_runner_args(mdb-nsim "--cores=${CONFIG_MP_NUM_CPUS}" "--nsim_args=${MDB_ARGS}")
board_runner_args(mdb-hw "--cores=${CONFIG_MP_NUM_CPUS}")

board_finalize_runner_args(arc-nsim)
include(${ZEPHYR_BASE}/boards/common/mdb-nsim.board.cmake)
include(${ZEPHYR_BASE}/boards/common/mdb-hw.board.cmake)
