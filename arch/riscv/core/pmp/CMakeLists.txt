#
# Copyright (c) 2020 BayLibre, SAS
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RISCV_PMP core_pmp.c)

zephyr_library_include_directories(
  .
  ../../include
)
