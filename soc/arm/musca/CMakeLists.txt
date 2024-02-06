#
# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_V2M_MUSCA_B1)
  zephyr_sources(b1/soc.c)
  zephyr_include_directories(b1)
elseif(CONFIG_SOC_V2M_MUSCA_S1)
  zephyr_include_directories(s1)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
