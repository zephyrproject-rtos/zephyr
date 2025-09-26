#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(${SOC_FAMILY})

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)

zephyr_sources_ifdef(CONFIG_NXP_NBU
  ../common/nxp_nbu.c
  )
