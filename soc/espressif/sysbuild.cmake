# Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_BOOTLOADER_MCUBOOT_ESPRESSIF)
  set(MCUBOOT_EP_IMAGE mcuboot_espressif_port)

  ExternalZephyrProject_Add(
    APPLICATION ${MCUBOOT_EP_IMAGE}
    SOURCE_DIR ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/espressif/zephyr/
  )

  set(SB_CONFIG_BOOTLOADER_MCUBOOT y CACHE INTERNAL "")
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BOOTLOADER_MCUBOOT y)

  # Below, the MCUboot configurations for bootloader and some for application build are set

  if(SB_CONFIG_ESP_MCUBOOT_BOOTLOADER_NO_DOWNGRADE)
    set_config_bool(${MCUBOOT_EP_IMAGE} CONFIG_ESP_DOWNGRADE_PREVENTION y)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_MCUBOOT_BOOTLOADER_NO_DOWNGRADE y)
  endif()

  if(SB_CONFIG_MCUBOOT_MODE_SWAP_USING_MOVE)
    set_config_bool(${MCUBOOT_EP_IMAGE} CONFIG_ESP_BOOT_SWAP_USING_MOVE y)
  elseif(SB_CONFIG_MCUBOOT_MODE_SWAP_SCRATCH)
    set_config_bool(${MCUBOOT_EP_IMAGE} CONFIG_ESP_BOOT_SWAP_USING_SCRATCH y)
  elseif(SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY)
    set_config_bool(${MCUBOOT_EP_IMAGE} CONFIG_ESP_BOOT_UPGRADE_ONLY y)
  endif()

  if(SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE)
    set_config_bool(${MCUBOOT_EP_IMAGE} CONFIG_ESP_SIGN_NONE y)
  else()
    set_config_string(${MCUBOOT_EP_IMAGE} CONFIG_ESP_SIGN_KEY_FILE
      "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")

    if(SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA)
      set_config_bool(${MCUBOOT_EP_IMAGE} CONFIG_ESP_SIGN_RSA y)
      if(SB_CONFIG_BOOT_SIGNATURE_RSA_LEN_2048)
        set_config_bool(${MCUBOOT_EP_IMAGE} CONFIG_ESP_SIGN_RSA_LEN_2048 y)
      elseif(SB_CONFIG_BOOT_SIGNATURE_RSA_LEN_3072)
        set_config_bool(${MCUBOOT_EP_IMAGE} CONFIG_ESP_SIGN_RSA_LEN_3072 y)
      endif()
    elseif(SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256)
      set_config_bool(${MCUBOOT_EP_IMAGE} CONFIG_ESP_SIGN_EC256 y)
    elseif(SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519)
      set_config_bool(${MCUBOOT_EP_IMAGE} CONFIG_ESP_SIGN_ED25519 y)
    endif()
  endif()

  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} ${MCUBOOT_EP_IMAGE})
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} ${MCUBOOT_EP_IMAGE})
endif()
