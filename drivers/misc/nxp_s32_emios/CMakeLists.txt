# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NXP_S32_EMIOS
  nxp_s32_emios.c
)
