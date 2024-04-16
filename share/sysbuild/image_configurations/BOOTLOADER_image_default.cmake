# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# This sysbuild CMake file sets the sysbuild controlled settings as properties
# on Zephyr MCUboot / bootloader image.

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

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  if("${SB_CONFIG_SIGNATURE_TYPE}" STREQUAL "NONE")
    set_config_bool(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE y)
  else()
    set_config_bool(${ZCMAKE_APPLICATION} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE n)
  endif()
endif()
