# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM nsim)

board_set_flasher_ifnset(arc-nsim)
board_set_debugger_ifnset(arc-nsim)

if(${CONFIG_SOC_NSIM_EM})
board_runner_args(arc-nsim "--props=nsim_em.props")
elseif(${CONFIG_SOC_NSIM_SEM})
board_runner_args(arc-nsim "--props=nsim_sem.props")
endif()

board_finalize_runner_args(arc-nsim)
