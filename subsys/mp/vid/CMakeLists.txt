#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MP_VID)
    zephyr_library_sources(
        mp_vid_buffer_pool.c
        mp_vid_object.c
        mp_vid_src.c
        mp_vid_transform.c
    )

    zephyr_library_sources_ifdef(CONFIG_MP_VID_CONVERT
        mp_vid_convert.c
        mp_vid_convert_table.c
    )

    zephyr_library_sources_ifdef(CONFIG_MP_RPC
        mp_vid_buffer_pool_client.c
        mp_vid_transform_client.c
    )
endif()
