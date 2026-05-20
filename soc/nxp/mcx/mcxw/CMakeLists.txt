# Copyright 2023-2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common/CMakeLists.txt)

zephyr_sources_ifdef(CONFIG_NXP_NBU
  ../../common/nxp_nbu.c
  )
