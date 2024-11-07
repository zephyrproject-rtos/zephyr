#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/board_version.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/error_handler.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/i2s_handler.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/nrf5340_audio_dk.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/custom_btn_handler.c
)
