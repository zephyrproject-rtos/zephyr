# Copyright 2022-2023 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(osif.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_NXP_S32_MC_RGM_ENABLED mc_rgm.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_NXP_S32_MC_ME_ENABLED mc_me.c)
