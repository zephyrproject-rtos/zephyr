# SPDX-License-Identifier: Apache-2.0
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

zephyr_library()

zephyr_library_sources(
  prep_c.c
  thread.c
  cpu_idle.c
  fatal.c
  switch.S
  irq_manage.c
  intc.c
  event_handlers.S
)


zephyr_library_sources(hvm_event_vectors.S)
zephyr_library_sources(vmsetie_cached.c)

# Enable TLS support - enhanced thread structure is now integrated
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
