# Copyright (c) 2023-2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# This sysbuild CMake file sets the sysbuild controlled settings as properties
# on Zephyr MCUboot / bootloader image.

set(bootmodes CONFIG_SINGLE_APPLICATION_SLOT
              CONFIG_BOOT_SWAP_USING_SCRATCH
              CONFIG_BOOT_UPGRADE_ONLY
              CONFIG_BOOT_SWAP_USING_MOVE
              CONFIG_BOOT_DIRECT_XIP
              CONFIG_BOOT_RAM_LOAD
              CONFIG_BOOT_FIRMWARE_LOADER)

if(SB_CONFIG_MCUBOOT_MODE_SINGLE_APP)
  set(bootmode CONFIG_SINGLE_APPLICATION_SLOT)
elseif(SB_CONFIG_MCUBOOT_MODE_SWAP_WITHOUT_SCRATCH OR SB_CONFIG_MCUBOOT_MODE_SWAP_USING_MOVE)
  set(bootmode CONFIG_BOOT_SWAP_USING_MOVE)
elseif(SB_CONFIG_MCUBOOT_MODE_SWAP_SCRATCH)
  set(bootmode CONFIG_BOOT_SWAP_USING_SCRATCH)
elseif(SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY)
  set(bootmode CONFIG_BOOT_UPGRADE_ONLY)
elseif(SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP OR SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT)
  set(bootmode CONFIG_BOOT_DIRECT_XIP)
elseif(SB_CONFIG_MCUBOOT_MODE_RAM_LOAD)
  set(bootmode CONFIG_BOOT_RAM_LOAD)
elseif(SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER)
  set(bootmode CONFIG_BOOT_FIRMWARE_LOADER)
endif()

foreach(loopbootmode ${bootmodes})
  if("${loopbootmode}" STREQUAL "${bootmode}")
    set_config_bool(${ZCMAKE_APPLICATION} ${loopbootmode} y)
  else()
    set_config_bool(${ZCMAKE_APPLICATION} ${loopbootmode} n)
  endif()
endforeach()

if(SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT)
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_BOOT_DIRECT_XIP_REVERT y)
else()
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_BOOT_DIRECT_XIP_REVERT n)
endif()

set(keytypes CONFIG_BOOT_SIGNATURE_TYPE_NONE
             CONFIG_BOOT_SIGNATURE_TYPE_RSA
             CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256
             CONFIG_BOOT_SIGNATURE_TYPE_ED25519)

if(SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE)
  set(keytype CONFIG_BOOT_SIGNATURE_TYPE_NONE)
elseif(SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA)
  set(keytype CONFIG_BOOT_SIGNATURE_TYPE_RSA)
elseif(SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256)
  set(keytype CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256)
elseif(SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519)
  set(keytype CONFIG_BOOT_SIGNATURE_TYPE_ED25519)
endif()

foreach(loopkeytype ${keytypes})
  if("${loopkeytype}" STREQUAL "${keytype}")
    set_config_bool(${ZCMAKE_APPLICATION} ${loopkeytype} y)
  else()
    set_config_bool(${ZCMAKE_APPLICATION} ${loopkeytype} n)
  endif()
endforeach()
