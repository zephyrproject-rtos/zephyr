set(SUPPORTED_EMU_PLATFORMS renode)
set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/it8xxx2_evb.resc)
board_set_flasher_ifnset(misc-flasher)
board_finalize_runner_args(misc-flasher)
