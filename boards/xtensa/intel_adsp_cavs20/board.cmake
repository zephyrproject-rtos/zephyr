# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(misc-flasher)
board_finalize_runner_args(misc-flasher)

if(CONFIG_BOARD_INTEL_ADSP_CAVS20)
board_set_rimage_target(icl)
endif()

if(CONFIG_BOARD_INTEL_ADSP_CAVS20_JSL)
board_set_rimage_target(jsl)
endif()
