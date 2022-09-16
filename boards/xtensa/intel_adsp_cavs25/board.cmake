# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(misc-flasher)
board_finalize_runner_args(misc-flasher)

if(CONFIG_BOARD_INTEL_ADSP_CAVS25)
board_set_rimage_target(tgl)
endif()

if(CONFIG_BOARD_INTEL_ADSP_CAVS25_TGPH)
board_set_rimage_target(tgl-h)
endif()
