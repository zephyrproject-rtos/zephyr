# Intel ACE SoC family CMake file
#
# Copyright (c) 2022-2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0


zephyr_library_sources(
  sram.c
  multiprocessing.c
  irq.c
  power_down.S
  power.c
  boot.c
  timestamp.c
  )

zephyr_include_directories(include)
zephyr_include_directories(include/${CONFIG_SOC})
zephyr_library_sources_ifdef(CONFIG_SOC_INTEL_COMM_WIDGET comm_widget.c)
zephyr_library_sources_ifdef(CONFIG_SOC_INTEL_COMM_WIDGET comm_widget_messages.c)

if (CONFIG_XTENSA_MMU)
  zephyr_library_sources_ifdef(CONFIG_SOC_INTEL_ACE30_PTL mmu_ptl.c)
endif()

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
