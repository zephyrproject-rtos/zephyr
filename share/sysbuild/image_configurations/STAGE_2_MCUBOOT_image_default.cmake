# Copyright (c) 2023-2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# This sysbuild CMake file sets the sysbuild controlled settings as properties on the 2nd stage
# MCUboot image.

set_config_bool(${ZCMAKE_APPLICATION} CONFIG_BOOTLOADER_MCUBOOT y)
set_config_string(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_SIGNATURE_KEY_FILE
                  "${SB_CONFIG_STAGE1_BOOT_SIGNATURE_KEY_FILE}"
)

if(SB_CONFIG_STAGE1_BOOT_SIGNATURE_TYPE_NONE)
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE y)
else()
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE n)
endif()

set_config_bool(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP y)
