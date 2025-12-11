# Copyright (c) 2022 Meta
#
# SPDX-License-Identifier: Apache-2.0
zephyr_sources_ifdef(CONFIG_SYS_HASH_FUNC32_DJB2 hash_func32_djb2.c)
zephyr_sources_ifdef(CONFIG_SYS_HASH_FUNC32_MURMUR3 hash_func32_murmur3.c)

zephyr_sources_ifdef(CONFIG_SYS_HASH_MAP_SC hash_map_sc.c)
zephyr_sources_ifdef(CONFIG_SYS_HASH_MAP_OA_LP hash_map_oa_lp.c)
zephyr_sources_ifdef(CONFIG_SYS_HASH_MAP_CXX hash_map_cxx.cpp)
