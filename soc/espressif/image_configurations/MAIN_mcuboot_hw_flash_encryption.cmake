# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

# Espressif main application image configuration for hardware flash encryption.

set_config_bool(${ZCMAKE_APPLICATION} CONFIG_ESP_FLASH_ENCRYPTION y)
set_config_string(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS
                  "--pad --align 32 --max-align 32")
