# Copyright (c) 2024 EPAM Systems
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_DT_HAS_RENESAS_RCAR_PFC_ENABLED)
  zephyr_library_sources(pfc_rcar.c)
elseif(CONFIG_DT_HAS_RENESAS_RCAR_PFC_X5H_ENABLED)
  zephyr_library_sources(pfc_rcar_x5h.c)
endif()

if(CONFIG_SOC_R8A77951_R7 OR CONFIG_SOC_R8A77951_A57)
  zephyr_library_sources(pfc_r8a77951.c)
elseif(CONFIG_SOC_R8A779F0_R52 OR CONFIG_SOC_R8A779F0_A55)
  zephyr_library_sources(pfc_r8a779f0.c)
elseif(CONFIG_SOC_R8A779G0_R52)
  zephyr_library_sources(pfc_r8a779g0.c)
endif()

zephyr_library_sources_ifdef(CONFIG_SOC_R8A77961 pfc_r8a77961.c)
zephyr_library_sources_ifdef(CONFIG_SOC_R8A78000_R52 pfc_r8a78000.c)
