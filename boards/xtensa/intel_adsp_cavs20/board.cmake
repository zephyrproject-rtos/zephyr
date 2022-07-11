# SPDX-License-Identifier: Apache-2.0

if($ENV{CAVS_OLD_FLASHER})
  board_set_flasher_ifnset(misc-flasher)
  board_finalize_runner_args(misc-flasher)
endif()

board_set_flasher_ifnset(intel_adsp)

set(RIMAGE_SIGN_KEY otc_private_key.pem)

if(CONFIG_BOARD_INTEL_ADSP_CAVS20)
board_set_rimage_target(icl)
endif()

if(CONFIG_BOARD_INTEL_ADSP_CAVS20_JSL)
board_set_rimage_target(jsl)
endif()

include(${ZEPHYR_BASE}/boards/common/intel_adsp.board.cmake)
