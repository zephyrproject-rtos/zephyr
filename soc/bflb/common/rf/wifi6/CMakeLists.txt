# Copyright (c) 2026 MASSDRIVER EI
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  platform.c
  wl80211_platform.c
)

zephyr_linker_sources(SECTIONS sections.ld)
