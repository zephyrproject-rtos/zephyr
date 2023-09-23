# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_MEM_ATTR mem_attr.c)
zephyr_sources_ifdef(CONFIG_MEM_ATTR_HEAP mem_attr_heap.c)
