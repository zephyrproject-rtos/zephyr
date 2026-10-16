# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LORAWAN_NVM_SETTINGS)
  zephyr_library()
  zephyr_library_sources(lorawan_nvm_settings.c)
endif()
