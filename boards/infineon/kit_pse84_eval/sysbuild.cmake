# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_BOARD_KIT_PSE84_EVAL_PSE846GPS2DBZC4A_M55)
  ExternalZephyrProject_Add(
    APPLICATION enable_cm55
    SOURCE_DIR ${ZEPHYR_BASE}/samples/basic/minimal
    BOARD kit_pse84_eval/pse846gps2dbzc4a/m33
  )

  set_config_bool(enable_cm55 CONFIG_SOC_PSE84_M55_ENABLE 1)
endif()
