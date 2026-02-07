# Copyright (c) 2026 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_S400)
  zephyr_sources(
    startup.S
    soc.c
    )
endif()
