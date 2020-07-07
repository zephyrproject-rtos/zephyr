# SPDX-License-Identifier: Apache-2.0
set(EMU_PLATFORM nsim)

if(NOT CONFIG_SOC_NSIM_HS_SMP)
board_set_flasher_ifnset(arc-nsim)
board_set_debugger_ifnset(arc-nsim)
endif()

if(${CONFIG_SOC_NSIM_EM})
board_runner_args(arc-nsim "--props=nsim_em.props")
board_runner_args(mdb "--nsim_args=mdb_em.args")
elseif(${CONFIG_SOC_NSIM_SEM})
board_runner_args(arc-nsim "--props=nsim_sem.props")
board_runner_args(mdb "--nsim_args=mdb_sem.args")
elseif(${CONFIG_SOC_NSIM_HS})
board_runner_args(arc-nsim "--props=nsim_hs.props")
board_runner_args(mdb "--nsim_args=mdb_hs.args")
elseif(${CONFIG_SOC_NSIM_HS_SMP})
board_runner_args(mdb "--cores=2" "--nsim_args=mdb_hs_smp.args")
endif()

board_finalize_runner_args(arc-nsim)
include(${ZEPHYR_BASE}/boards/common/mdb.board.cmake)