# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
  osal_heap.c
  osal_lock.c
  osal_mutex.c
  osal_sem.c
  osal_queue.c
  osal_fifo.c
  osal_event.c
  osal_mempool.c
  osal_thread.c
  osal_time.c
  osal_timer.c
)
