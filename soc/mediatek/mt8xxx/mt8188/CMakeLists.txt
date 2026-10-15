# Copyright 2024 The ChromiumOS Authors
# Copyright (c) 2026 MediaTek Inc.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MT8188_ADSP)
  # soc.h, included by arch/xtensa/irq.h, lives in the cluster dir.
  zephyr_include_directories(adsp)

  zephyr_library_sources(adsp/cpuclk.c)

  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/adsp/linker.ld CACHE INTERNAL "")
endif()

if(CONFIG_SOC_MT8188_A55)
  # pinctrl_soc.h, included by the pinctrl framework, lives in the cluster dir.
  zephyr_include_directories(a55)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
endif()
