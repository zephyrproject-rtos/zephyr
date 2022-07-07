# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(intel_adsp)

set(RIMAGE_SIGN_KEY otc_private_key.pem)

board_set_rimage_target(apl)

include(${ZEPHYR_BASE}/boards/common/intel_adsp.board.cmake)
