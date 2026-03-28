# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# This sysbuild CMake file sets the sysbuild controlled settings as properties
# on a firmware updater image.

set_config_bool(${ZCMAKE_APPLICATION} CONFIG_BOOTLOADER_MCUBOOT "${SB_CONFIG_BOOTLOADER_MCUBOOT}")
set_config_string(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_SIGNATURE_KEY_FILE
                  "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}"
)
set_config_string(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE
                  "${SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE}"
)

if("${SB_CONFIG_SIGNATURE_TYPE}" STREQUAL "NONE")
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE y)
else()
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE n)
endif()

set_config_bool(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_BOOTLOADER_MODE_FIRMWARE_UPDATER y)
