# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(nordic_vpr_launcher.c)

if(CONFIG_NRF_REMOTE_IMAGE_INC_DIR)
  zephyr_include_directories(${CONFIG_NRF_REMOTE_IMAGE_INC_DIR})
endif()
