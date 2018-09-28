set(EMU_PLATFORM nsim)

set(BOARD_FLASH_RUNNER arc-nsim)
set(BOARD_DEBUG_RUNNER arc-nsim)

if(${CONFIG_SOC_NSIM_EM})
board_runner_args(arc-nsim "--props=nsim.props")
elseif(${CONFIG_SOC_NSIM_SEM})
board_runner_args(arc-nsim "--props=nsim_sem.props")
endif()

board_finalize_runner_args(arc-nsim)
