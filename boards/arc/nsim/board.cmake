# SPDX-License-Identifier: Apache-2.0

if(${CONFIG_SOC_NSIM_HS_SMP})
set(EMU_PLATFORM mdb)
else()
set(EMU_PLATFORM nsim)
endif()

if(NOT ${CONFIG_SOC_NSIM_HS_SMP})
board_set_flasher_ifnset(arc-nsim)
board_set_debugger_ifnset(arc-nsim)
endif()

if(${CONFIG_SOC_NSIM_EM})
board_runner_args(arc-nsim "--props=nsim_em.props")
elseif(${CONFIG_SOC_NSIM_SEM})
board_runner_args(arc-nsim "--props=nsim_sem.props")
elseif(${CONFIG_SOC_NSIM_HS})
board_runner_args(arc-nsim "--props=nsim_hs.props")
endif()

board_finalize_runner_args(arc-nsim)
