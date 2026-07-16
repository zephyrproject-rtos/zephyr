# Copyright 2026 sensry.io
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_VIDEO_HEIMANN_HTPA)
  zephyr_library_sources(
    video_htpa_calib.c
    video_htpa_proc.c
    video_htpa_sensor.c
  )
endif()
