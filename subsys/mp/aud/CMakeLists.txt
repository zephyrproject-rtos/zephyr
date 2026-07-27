#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MP_AUD)
  zephyr_library_sources(
    mp_aud_buffer_pool.c
    mp_aud_dmic_src.c
    mp_aud_gain.c
    mp_aud_i2s_codec_sink.c
    mp_aud_src.c
    mp_aud.c
  )
endif()
