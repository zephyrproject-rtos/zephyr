# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  heap.c
  )

zephyr_sources_ifdef(CONFIG_SYS_HEAP_RUNTIME_STATS heap_stats.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_INFO heap_info.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_VALIDATE heap_validate.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_STRESS heap_stress.c)
zephyr_sources_ifdef(CONFIG_SHARED_MULTI_HEAP shared_multi_heap.c)
zephyr_sources_ifdef(CONFIG_MULTI_HEAP multi_heap.c)
zephyr_sources_ifdef(CONFIG_HEAP_LISTENER heap_listener.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_ARRAY_SIZE heap_array.c)
