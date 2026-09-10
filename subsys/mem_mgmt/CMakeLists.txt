# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_DEMAND_PAGING demand_paging)

zephyr_sources_ifdef(CONFIG_MEM_ATTR mem_attr.c)
zephyr_sources_ifdef(CONFIG_MEM_ATTR_HEAP mem_attr_heap.c)
