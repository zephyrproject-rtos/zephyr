#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MPIPE_AUD)
  zephyr_library_sources(
    mpipe_aud_buffer_pool.c
    mpipe_aud_dmic_src.c
    mpipe_aud_gain.c
    mpipe_aud_i2s_codec_sink.c
    mpipe_aud_src.c
    mpipe_aud.c
  )
endif()
