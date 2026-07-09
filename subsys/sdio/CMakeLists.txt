# Copyright 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SDIO)
  zephyr_library()
  zephyr_library_sources(sdio_core.c)
  zephyr_library_sources_ifdef(CONFIG_SDIO_HOST sdio_host.c)
endif()
