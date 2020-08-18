# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})

zephyr_sources(
  validate_base_addresses.c
  validate_enabled_instances.c
  )

zephyr_library_sources_ifdef(CONFIG_TIMING_FUNCTIONS timing.c)
