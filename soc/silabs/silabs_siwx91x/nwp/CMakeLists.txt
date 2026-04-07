# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SILABS_SIWX91X_NWP_WISECONNECT)
  zephyr_sources(siwx91x_nwp.c)
  zephyr_include_directories(.)
endif()
