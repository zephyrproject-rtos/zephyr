# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  cpu_idle.c
  irq.c
)

add_subdirectory(${SOC_SERIES})
