# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

if(NOT SB_CONFIG_MCUBOOT_ESP_FLASH_ENCRYPTION)
  return()
endif()

set(espressif_image_conf_dir ${CMAKE_CURRENT_LIST_DIR}/image_configurations)

if(SB_CONFIG_BOOTLOADER_MCUBOOT AND TARGET mcuboot)
  get_property(tmp_conf_scripts TARGET mcuboot PROPERTY IMAGE_CONF_SCRIPT)
  list(APPEND tmp_conf_scripts
       "${espressif_image_conf_dir}/BOOTLOADER_mcuboot_hw_flash_encryption.cmake")
  set_target_properties(mcuboot PROPERTIES IMAGE_CONF_SCRIPT "${tmp_conf_scripts}")
endif()

if(TARGET ${DEFAULT_IMAGE})
  get_property(tmp_conf_scripts TARGET ${DEFAULT_IMAGE} PROPERTY IMAGE_CONF_SCRIPT)
  list(APPEND tmp_conf_scripts
       "${espressif_image_conf_dir}/MAIN_mcuboot_hw_flash_encryption.cmake")
  set_target_properties(${DEFAULT_IMAGE} PROPERTIES IMAGE_CONF_SCRIPT "${tmp_conf_scripts}")
endif()
