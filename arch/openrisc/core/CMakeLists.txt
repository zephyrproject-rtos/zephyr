#
# Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources(
  cpu_idle.c
  exception.S
  fatal.c
  irq_manage.c
  irq_offload.c
  prep_c.c
  reboot.c
  switch.S
  thread.c
)

zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
