# SPDX-License-Identifier: Apache-2.0

if((CONFIG_BOARD_RAYTAC_MDBT53V_DB_40_NRF5340_CPUAPP OR CONFIG_BOARD_RAYTAC_MDBT53V_DB_40_NRF5340_CPUAPP_NS)
   AND CONFIG_BOARD_ENABLE_CPUNET)
  zephyr_library()
  zephyr_library_sources(raytac_mdbt53v_db_40_nrf5340_cpunet_reset.c)
endif()
