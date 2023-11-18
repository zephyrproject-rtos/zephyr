# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  heap.c
  heap-info.c
  multi_heap.c
  )

zephyr_sources_ifdef(CONFIG_SYS_HEAP_VALIDATE heap-validate.c)
zephyr_sources_ifdef(CONFIG_SYS_HEAP_STRESS heap-stress.c)
zephyr_sources_ifdef(CONFIG_SHARED_MULTI_HEAP shared_multi_heap.c)

zephyr_sources_ifdef(CONFIG_HEAP_LISTENER heap_listener.c)
