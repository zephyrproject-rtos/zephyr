#
# Copyright (c) 2019, Manivannan Sadhasivam <mani@kernel.org>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  soc_reset.c
  soc_pinmux.c
  soc_pmu.c
  soc_clock.c
  )

zephyr_linker_sources_ifdef(CONFIG_ATB110X_BOOT_HEADER
  ROM_START SORT_KEY 0xc0 boot_header.ld)
