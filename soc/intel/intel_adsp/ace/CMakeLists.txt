# Intel ACE SoC family CMake file
#
# Copyright (c) 2022 Intel Corporation
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
zephyr_include_directories(include/${SOC_NAME})
zephyr_library_sources_ifdef(CONFIG_SOC_INTEL_COMM_WIDGET comm_widget.c)
zephyr_library_sources_ifdef(CONFIG_SOC_INTEL_COMM_WIDGET comm_widget_messages.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
