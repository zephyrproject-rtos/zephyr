# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_APP_USE_CPU1)
  set(${DEFAULT_IMAGE}_EXTRA_DTC_OVERLAY_FILE ${APP_DIR}/dual.overlay
      CACHE INTERNAL "MPU-aligned private CPU0 SRAM")
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD frdm_mcxe32b/mcxe32b/cpu1
  )
  set_config_bool(${DEFAULT_IMAGE} CONFIG_APP_USE_CPU1 y)
  set_config_int(${DEFAULT_IMAGE} CONFIG_FLASH_LOAD_SIZE 0x200000)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)
endif()
