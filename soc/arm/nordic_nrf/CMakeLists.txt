# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})

zephyr_sources(
  validate_base_addresses.c
  nrf_reset_reason.c
  )
