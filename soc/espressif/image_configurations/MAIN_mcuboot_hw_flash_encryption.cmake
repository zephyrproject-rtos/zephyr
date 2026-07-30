# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

# Espressif main application image configuration for hardware flash encryption.

set_config_bool(${ZCMAKE_APPLICATION} CONFIG_ESP32_FLASH_ENCRYPTION y)

if(SB_CONFIG_SOC_FAMILY_ESPRESSIF_EFUSE_VIRTUAL)
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_SOC_FAMILY_ESPRESSIF_EFUSE_VIRTUAL y)
endif()

if(SB_CONFIG_SOC_FAMILY_ESPRESSIF_EFUSE_VIRTUAL_KEEP_IN_FLASH)
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_SOC_FAMILY_ESPRESSIF_EFUSE_VIRTUAL_KEEP_IN_FLASH y)
endif()
