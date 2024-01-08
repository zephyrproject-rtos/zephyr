#
# Copyright (c) 2021, Kwon Tae-young <tykwon@m2i.co.kr>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  )

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
