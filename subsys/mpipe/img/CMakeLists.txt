#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MPIPE_IMG)
  zephyr_library_sources(
    mpipe_img_jpeg_parser.c
    mpipe_img_jpeg_decoder.c
    jpeg_dec.c
  )
endif()
