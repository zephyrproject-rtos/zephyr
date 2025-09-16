# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NRF_PERIPHCONF_SECTION)
  zephyr_linker_sources(SECTIONS uicr.ld)
endif()
