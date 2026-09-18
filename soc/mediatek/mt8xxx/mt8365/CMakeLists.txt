# Copyright 2024 The ChromiumOS Authors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MT8365_ADSP)
  # soc.h, included by arch/xtensa/irq.h, lives in the cluster dir.
  zephyr_include_directories(adsp)

  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/adsp/linker.ld CACHE INTERNAL "")
endif()
