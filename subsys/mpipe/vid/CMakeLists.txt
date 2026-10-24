#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MPIPE_VID)
  zephyr_library_sources(
    mpipe_vid_buffer_pool.c
    mpipe_vid_object.c
    mpipe_vid_src.c
    mpipe_vid_transform.c
    )

  zephyr_library_sources_ifdef(CONFIG_MPIPE_VID_CONVERT
    mpipe_vid_convert.c
    mpipe_vid_convert_table.c
  )

  zephyr_library_sources_ifdef(CONFIG_MPIPE_RPC
    mpipe_vid_buffer_pool_client.c
    mpipe_vid_transform_client.c
  )
endif()
