# Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_RA_SECOND_CORE_LAUNCHER)
  set(image "second_core_launcher")

  set(qualifiers ${CONFIG_SOC}/${SB_CONFIG_RA_SECOND_CORE_LAUNCHER_NAME})

  ExternalZephyrProject_Add(
    APPLICATION ${image}
    SOURCE_DIR ${ZEPHYR_BASE}/samples/basic/minimal
    BOARD ${BOARD}/${qualifiers}
  )

  set_config_bool(${image} CONFIG_SOC_RA_ENABLE_START_SECOND_CORE 1)
endif()
