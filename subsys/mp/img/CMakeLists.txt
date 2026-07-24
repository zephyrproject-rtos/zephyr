#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MP_IMG)
    zephyr_library_sources(
        mp_img_jpeg_parser.c
        mp_img_jpeg_decoder.c
        jpeg_dec.c
    )
endif()
