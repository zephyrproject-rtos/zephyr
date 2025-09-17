# Copyright (c) 2026 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_S420)
  zephyr_sources(
    startup.S
    soc.c
    )

zephyr_sources_ifdef(CONFIG_SMP
     ipi.c
    )
endif()
