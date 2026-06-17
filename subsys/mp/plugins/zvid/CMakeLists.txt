#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_sources(
    mp_zvid_buffer_pool.c
    mp_zvid_object.c
    mp_zvid_src.c
    mp_zvid_transform.c
)

zephyr_library_sources_ifdef(CONFIG_MP_ZVID_CONVERT
    mp_zvid_convert.c
    mp_zvid_convert_table.c
)

zephyr_library_sources_ifdef(CONFIG_MP_RPC
    mp_zvid_buffer_pool_client.c
    mp_zvid_transform_client.c
)
