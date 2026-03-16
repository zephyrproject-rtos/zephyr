# Copyright 2023-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})

zephyr_sources_ifdef(CONFIG_NXP_NBU
  ../../common/nxp_nbu.c
  )
