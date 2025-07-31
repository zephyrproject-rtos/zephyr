# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MEMC_NXP_EMC)
  zephyr_library()
  zephyr_library_sources(memc_nxp_emc.c)
endif()