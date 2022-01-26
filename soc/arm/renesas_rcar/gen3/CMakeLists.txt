# Copyright (c) 2021 IoT.bzh
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
)
zephyr_library_sources_ifdef(CONFIG_SOC_R8A77951 pfc_r8a77951.c)
