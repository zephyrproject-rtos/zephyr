# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS acesim)

board_set_rimage_target(lnl)

set(RIMAGE_SIGN_KEY "otc_private_key_3k.pem" CACHE STRING "default in ace20_lnl/board.cmake")
