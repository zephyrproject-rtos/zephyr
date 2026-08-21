# SPDX-License-Identifier: Apache-2.0
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

zephyr_library()

zephyr_library_sources(
  hvm_event_vectors.S
  event_handlers.S
  switch.S
  prep_c.c
  thread.c
  cpu_idle.c
  fatal.c
  irq_manage.c
  intc.c
  mem_manage.c
)

zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
