# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(DEFINED SB_CONFIG_REMOTE_BOARD)
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_REMOTE_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )

  if(SB_CONFIG_SOC_SERIES_NRF53)
    set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPUNET)
    set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES remote)
    set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET remote)
    set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")
  else(SB_CONFIG_SOC_SERIES_NRF54L)
    set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPUFLPR)
    set_property(GLOBAL APPEND PROPERTY PM_CPUFLPR_IMAGES remote)
    set_property(GLOBAL PROPERTY DOMAIN_APP_CPUFLPR remote)
    set(CPUFLPR_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")
  endif()

  add_dependencies(${DEFAULT_IMAGE} remote)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)
endif()
