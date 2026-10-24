#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MPIPE_IMG)
  zephyr_library_sources(mpipe_img_jpeg_parser.c)
  zephyr_library_sources_ifdef(CONFIG_MPIPE_IMG_JPEG_DECODER mpipe_img_jpeg_decoder.c)
endif()
