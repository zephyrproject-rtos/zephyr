# Intel ACE SoC family CMake file
#
# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_PM
  power.c
  )

zephyr_library_sources(
  soc.c
  multiprocessing.c
  irq.c
  power_down.S
  )
