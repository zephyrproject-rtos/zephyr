# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

# Application image does not enable Secure Boot. When flash encryption is
# off, virtual eFuse still has to be forwarded so the app and MCUboot share
# the same emulated eFuse region.

if(SB_CONFIG_SOC_FAMILY_ESPRESSIF_EFUSE_VIRTUAL)
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_SOC_FAMILY_ESPRESSIF_EFUSE_VIRTUAL y)
endif()

if(SB_CONFIG_SOC_FAMILY_ESPRESSIF_EFUSE_VIRTUAL_KEEP_IN_FLASH)
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_SOC_FAMILY_ESPRESSIF_EFUSE_VIRTUAL_KEEP_IN_FLASH y)
endif()
